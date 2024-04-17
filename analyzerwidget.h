#ifndef ANALYZERWIDGET_H
#define ANALYZERWIDGET_H

#include <QWidget>
#include <QtWidgets>
#include "graphwidget.h"
#include "objnetdevice.h"

using namespace Objnet;

class AnalyzerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AnalyzerWidget(ObjnetDevice *dev, QWidget *parent = nullptr);
    virtual ~AnalyzerWidget();

//    void addPoint(QString name, float time, float value);
    void clear();

//    void analyze();


signals:

private:
    ObjnetDevice *m_device;
    GraphWidget *m_graph, *m_phaseGraph;
//    QMap<QString, QVector<QPointF>> m_points;
    QTimer *m_requestTimer;
//    QLineEdit *m_pointCount;

    QDoubleSpinBox *m_freqStartSpin;
    QDoubleSpinBox *m_freqEndSpin;
    QDoubleSpinBox *m_ampSpin;
    QDoubleSpinBox *m_durSpin;
    QSpinBox *m_filterSpin;
    QPushButton *m_btnStartStop;
    QComboBox *m_inputBox;

    float m_freqStart = 0;
    float m_freqEnd = 0;
    float m_amp = 0;
    float m_dur = 0;
    bool m_active = false;
//    std::vector<std::string> m_inputNames;
    QList<QPointF> m_filter;

    void onDeviceReady();
    void onSampleReceived(QVariantMap values);
    void onObjectChanged(QString name, QVariant value);
};

#endif // ANALYZERWIDGET_H
