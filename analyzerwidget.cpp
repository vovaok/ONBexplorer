#include "analyzerwidget.h"

AnalyzerWidget::AnalyzerWidget(ObjnetDevice *dev, QWidget *parent)
    : QWidget(parent, Qt::Tool),
    m_device(dev)
{
    connect(m_device, &ObjnetDevice::ready, this, &AnalyzerWidget::onDeviceReady);

    m_graph = new GraphWidget;
    m_graph->setMinimumSize(320, 240);
    m_graph->setMaxPointCount(100000);
    m_graph->setAutoBoundsEnabled(true);
    m_graph->addGraph("magnitude", Qt::red, 2.0f);
    m_graph->addGraph("unfiltered", QColor(255, 0, 0, 64), 1.0f);
//    m_graph->setGraphType("magnitude", Graph::Points);
//    m_graph->setPointSize("magnitude", 3.0);

    m_phaseGraph = new GraphWidget;
    m_phaseGraph->setMinimumSize(320, 120);
    m_phaseGraph->setMaxPointCount(100000);
    m_phaseGraph->setAutoBoundsEnabled(true);
    m_phaseGraph->addGraph("phase", Qt::blue, 2.0f);
//    m_phaseGraph->setGraphType("phase", Graph::Points);
//    m_phaseGraph->setPointSize("phase", 3.0);

    connect(m_graph, &GraphWidget::boundsChanged, [=]()
    {
        m_phaseGraph->setXmin(m_graph->minX());
        m_phaseGraph->setXmax(m_graph->maxX());
        m_phaseGraph->update();
    });

    connect(m_phaseGraph, &GraphWidget::boundsChanged, [=]()
    {
        m_graph->setXmin(m_phaseGraph->minX());
        m_graph->setXmax(m_phaseGraph->maxX());
        m_graph->update();
    });

//    QPushButton *btnAnal = new QPushButton("analyze");
//    connect(btnAnal, &QPushButton::clicked, this, &AnalyzerWidget::analyze);

//    m_pointCount = new QLineEdit();
//    m_pointCount->setReadOnly(true);

    m_freqStartSpin = new QDoubleSpinBox();
    m_freqStartSpin->setRange(1, 1000);
    m_freqStartSpin->setDecimals(0);

    m_freqEndSpin = new QDoubleSpinBox();
    m_freqEndSpin->setRange(1, 10000);
    m_freqEndSpin->setDecimals(0);

    m_ampSpin = new QDoubleSpinBox();
    m_ampSpin->setRange(0, 1000);
    m_ampSpin->setDecimals(1);

    m_durSpin = new QDoubleSpinBox();
    m_durSpin->setRange(0.001, 1000);
    m_durSpin->setDecimals(3);

    m_filterSpin = new QSpinBox();
    m_filterSpin->setRange(0, 50);
    m_filterSpin->setValue(5);

    m_inputBox = new QComboBox();

    m_btnStartStop = new QPushButton("Start/stop");

    QPushButton *btnClear = new QPushButton("Clear");
    connect(btnClear, &QPushButton::clicked, this, &AnalyzerWidget::clear);

    QSplitter *split = new QSplitter(Qt::Vertical);
    split->addWidget(m_graph);
    split->addWidget(m_phaseGraph);
    split->setStretchFactor(0, 2);
    split->setStretchFactor(1, 1);

    QVBoxLayout *lay = new QVBoxLayout;
    QGridLayout *btnlay = new QGridLayout;
    setLayout(lay);
    lay->addLayout(btnlay);
    lay->addWidget(split);

    btnlay->addWidget(new QLabel("Start Frequency:"), 0, 0);
    btnlay->addWidget(m_freqStartSpin, 0, 1);
    btnlay->addWidget(new QLabel("End Frequency:"), 0, 2);
    btnlay->addWidget(m_freqEndSpin, 0, 3);
    btnlay->addWidget(new QLabel("Test amplitude:"), 1, 0);
    btnlay->addWidget(m_ampSpin, 1, 1);
    btnlay->addWidget(new QLabel("Duration:"), 1, 2);
    btnlay->addWidget(m_durSpin, 1, 3);
    btnlay->addWidget(new QLabel("Magnitude filter:"), 2, 0);
    btnlay->addWidget(m_filterSpin, 2, 1);

    btnlay->addWidget(btnClear, 3, 0);
    btnlay->addWidget(new QLabel("Input signal:"), 3, 1);
    btnlay->addWidget(m_inputBox, 3, 2);
    btnlay->addWidget(m_btnStartStop, 3, 3);

    m_requestTimer = new QTimer(this);
    m_requestTimer->setInterval(16);

    connect(m_requestTimer, &QTimer::timeout, [=]()
    {
        m_device->groupedRequest({"freq", "magnitude", "phase"});
    });

    connect(m_btnStartStop, &QPushButton::clicked, [=]()
    {
        m_device->requestObject("start/stop");
        m_filter.clear();
        m_graph->addPoint("magnitude", NAN, NAN);
        m_graph->addPoint("unfiltered", NAN, NAN);
        m_phaseGraph->addPoint("phase", NAN, NAN);
    });

    connect(m_freqStartSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double value)
    {
        m_device->sendObject("freqStart", value);
    });

    connect(m_freqEndSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double value)
    {
        m_device->sendObject("freqEnd", value);
    });

    connect(m_ampSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double value)
    {
        m_device->sendObject("targetAmplitude", value);
    });

    connect(m_durSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double value)
    {
        m_device->sendObject("duration", value);
    });

    connect(m_inputBox, &QComboBox::currentTextChanged, [=](QString text)
    {
        m_device->sendObject("set input", text);
    });
}

AnalyzerWidget::~AnalyzerWidget()
{
    m_requestTimer->stop();
}


void AnalyzerWidget::clear()
{
//    m_points.clear();
    m_graph->clear();
    m_phaseGraph->clear();
    m_graph->update();
    m_phaseGraph->update();
}

/*
void AnalyzerWidget::analyze()
{
    m_graph->clear();
    m_graph->setBounds(0, 0, 1, 1);
    m_phaseGraph->clear();
    m_phaseGraph->setBounds(0, 0, 1, 1);

    static QColor colors[] = {Qt::red, Qt::blue, Qt::black, Qt::darkGreen, Qt::darkMagenta, Qt::darkCyan, Qt::darkYellow, Qt::gray};
    int colidx = 0;

    for (QString &name: m_points.keys())
    {
        GraphWidget *graph = m_graph;
        if (name.contains("phase"))
            graph = m_phaseGraph;
        graph->addGraph(name, colors[colidx]);
        graph->setGraphType(name, Graph::Points);
        graph->setPointSize(name, 3.0);

        QVector<QPointF> &pts = m_points[name];
        for (QPointF &p: pts)
        {
            m_graph->addPoint(name, p.x(), p.y());
        }

    }

//    int aaa = 0;
//    for (QString &name: m_points.keys())
//    {
//        aaa++;
//        qDebug() << aaa;
//        if (aaa != 3)
//            continue;
//        QVector<QPointF> &pts = m_points[name];
//        QVector<bool> trig;
//        float olda = pts[0].y();
//        bool first = true;
//        bool desc = false;
//        for (QPointF &p: pts)
//        {
//            bool ref = false;

//            float a = p.y();
//            if (a > olda)
//            {
//                desc = false;
//            }
//            else if (!desc && a < olda)
//            {
//                desc = true;
//                ref = true;
//            }
//            olda = a;


//            trig << ref;

//            m_graph->addPoint(name, p.x(), p.y());
//            m_graph->addPoint("trig", p.x(), (ref? 10: 0));
//        }

//    }

//    for (QString &name: m_points.keys())
//    {
//        QVector<QPointF> &pts = m_points[name];
//        m_graph->addGraph(name, colors[colidx]);
//        m_phaseGraph->addGraph(name, colors[colidx]);
//        colidx++;
//        int N = pts.size();
//        float T = pts.last().x() - pts.first().x();
////        float dt = T / N;
//        float k = 2.f / N;

//        for (int i=1; i<=N/2; i++)
//        {
//            float a = 0, b = 0;
//            QPointF *p = pts.data();
//            for (int j=0; j<N; j++)
//            {
//                float y = p[j].y();
//                a += k * y * cosf(2*M_PI * i * j / N);
//                b += k * y * sinf(2*M_PI * i * j / N);
//            }
//            float f = i / T;
//            float m = sqrtf(a*a + b*b);
//            float ph = atan2f(b, a);
////            graph->addPoint(f, m);
//            m_graph->addPoint(name, f, m);
//            m_phaseGraph->addPoint(name, f, ph);
//        }
//    }

    m_graph->update();
    m_phaseGraph->update();
}
*/

void AnalyzerWidget::onDeviceReady()
{
    connect(m_device, &ObjnetDevice::objectGroupReceived, this, &AnalyzerWidget::onSampleReceived);
    connect(m_device, &ObjnetDevice::objectValueChanged, this, &AnalyzerWidget::onObjectChanged);

    m_device->bindVariable("freqStart", m_freqStart);
    m_device->bindVariable("freqEnd", m_freqEnd);
    m_device->bindVariable("targetAmplitude", m_amp);
    m_device->bindVariable("duration", m_dur);
    m_device->bindVariable("active", m_active);
//    m_device->bindVariable("inputs", m_inputNames);

    m_device->autoRequest("active", 10);
    m_device->requestObject("freqStart");
    m_device->requestObject("freqEnd");
    m_device->requestObject("targetAmplitude");
    m_device->requestObject("duration");

    m_device->requestObject("inputs");
//    m_device->requestObject("input");
}

void AnalyzerWidget::onSampleReceived(QVariantMap values)
{
    if (values.contains("freq"))
    {
        float f = values["freq"].toFloat();
        float m = values["magnitude"].toFloat();
        float ph = values["phase"].toFloat() * 180 / M_PI;

        f = log10f(f);

        m_graph->addPoint("unfiltered", f, m);
        m_phaseGraph->addPoint("phase", f, ph);

        m_filter << QPointF(f, m);
        int Nf = m_filterSpin->value() * 2 + 1;
        int n = 0;
        if (m_filter.size() > Nf)
        {
            m_filter.removeFirst();
            float v = 0;
            for (int i=0; i<Nf; i++)
            {
                float y = m_filter[i].y();
                if (isnan(y) || isinf(y))
                    continue;
                v += y;
                n++;
            }
            if (n)
                v /= n;
            m_graph->addPoint("magnitude", m_filter[Nf/2].x(), v);
        }

    }
    m_graph->update();
    m_phaseGraph->update();
}

void AnalyzerWidget::onObjectChanged(QString name, QVariant value)
{
    if (name == "active")
    {
        bool act = value.toBool();
        m_btnStartStop->setDown(act);
        if (act)
            m_requestTimer->start();
        else
            m_requestTimer->stop();
    }
    else if (name == "freqStart")
    {
        m_freqStartSpin->setValue(value.toFloat());
    }
    else if (name == "freqEnd")
    {
        m_freqEndSpin->setValue(value.toFloat());
    }
    else if (name == "targetAmplitude")
    {
        m_ampSpin->setValue(value.toFloat());
    }
    else if (name == "duration")
    {
        m_durSpin->setValue(value.toFloat());
    }
    else if (name == "inputs")
    {
        QStringList inputs = m_device->objectInfo("inputs")->toVariant().toStringList();
        m_inputBox->clear();
        m_inputBox->addItems(inputs);
    }
//    else if (name == "input")
//    {
//        m_inputBox->setCurrentText(value.toString());
//    }
}
