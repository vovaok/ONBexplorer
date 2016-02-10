#ifndef _OBJNET_COMMON_H
#define _OBJNET_COMMON_H

#include <string>
#include <typeinfo>
#ifdef __ICCARM__
#include "core/core.h"
#else
#include <QtCore>
#define ByteArray QByteArray
#endif

/*! @brief Object Network.
    This is a part of STM32++ library providing network connection between stm32 devices
*/
namespace Objnet
{

using namespace std;

//! Standard ActionID enumeration.
typedef enum
{
    aidPropagationUp    = 0x40, //!< пересылка сообщения через мастера на уровень выше
    aidPropagationDown  = 0x80, //!< пересылка сообщения через узлы на уровень ниже
    aidPollNodes        = 0x00, //!< опрос узлов, можно добавить пересылку с помощью ИЛИ
    aidConnReset        = 0x01 | aidPropagationDown, //!< сброс состояния узлов до disconnected, установка соединения заново
//    aidEnumerate        = 0x02, //!< построение карты сети

    aidUpgradeStart     = 0x30, //!< запуск обновления прошивки, в данных класс устройства
    aidUpgradeConfirm   = 0x31, //!< подтверждение начала прошивки, чтобы не было случайностей
    aidUpgradeEnd       = 0x32, //!< окончание обновления прошивки
    aidUpgradeData      = 0x34, //!< собственно, сама прошивка (см. протокол)
    aidUpgradeRepeat    = 0x38, //!< запрос повтора страницы
} StdAID;

//! Service ObjectID enumeration.
typedef enum
{
    svcClass            = 0x00, //!< узел говорит, из каких он слоёв общества
    svcName             = 0x01, //!< узел сообщает, как его величать
    svcFullName         = 0x02, //!< полное имя узла
    svcSerial           = 0x03, //!< серийный номер
    svcVersion          = 0x04, //!< версия прошивки
    svcBuildDate        = 0x05, //!< дата и время сборки
    svcCpuInfo          = 0x06, //!< информация о процессоре
    svcBurnCount        = 0x07, //!< без комментариев..
    svcObjectCount      = 0x08, //!< число объектов
    // можно добавить что-нибудь ещё..

    svcEcho             = 0xF0, //!< узел напоминает мастеру, что он здесь
    svcHello            = 0xF1, //!< сообщение узла, что он пришёл с приветом, либо мастер играет в юща и спрашивает узла: "А вы, собственно, кто?"
    svcWelcome          = 0xF2, //!< мастер приглашает в гости
    svcWelcomeAgain     = 0xF3, //!< мастер уже приглашал в гости
    svcConnected        = 0xF4, //!< мастер сообщает вверх, что пришёл девайс
    svcDisconnected     = 0xF5, //!< мастер понял, что девайс отключился, и передаёт по цепочке главному
    svcKill             = 0xF6, //!< мастеру надоело, что девайс в отключке, он его убивает и рассказывает об этом всем
} SvcOID;

typedef enum
{
    netAddrUniversal    = 0x7F,
    netAddrInvalid      = 0xFF
} NetAddress;

//! Local Message ID type.
struct LocalMsgId
{
    unsigned char oid;          //!< object ID
    unsigned char sender: 7;    //!< sender network address (logical)
    unsigned char frag: 1;      //!< message is fragmented
    unsigned char addr: 7;      //!< receiver network address (logical)
    unsigned char svc: 1;       //!< message is service
    unsigned char mac: 4;       //!< receiver bus address (physical)
    const unsigned char local: 1; //!< local area message (inside the bus) = 1
    /*! Конструктор обнуляет. */
    LocalMsgId() :
      oid(0), sender(0), frag(0), addr(0), svc(0), mac(0), local(1)
    {
    }
    /*! Неявный конструктор. */
    LocalMsgId(const unsigned long &data) :
      local(1)
    {
        *reinterpret_cast<unsigned long*>(this) = data;
    }
    /*! Приведение типа к unsigned long */
    operator unsigned long&() {return *reinterpret_cast<unsigned long*>(this);}
    /*! Приведение типа из unsigned long */
    void operator =(const unsigned long &data) {*reinterpret_cast<unsigned long*>(this) = data;}
};

//! Global Message ID type.
struct GlobalMsgId
{
    unsigned char aid;      //!< action ID
    unsigned char res: 8;   //!< reserved
    unsigned char addr: 7;  //!< own network address (logical)
    unsigned char svc: 1;   //!< message is service
    unsigned char mac: 4;   //!< own bus address (physical)
    unsigned char local: 1; //!< local area message (inside the bus) = 0
    /*! Конструктор обнуляет. */
    GlobalMsgId() :
      aid(0), res(0), addr(0), svc(0), mac(0), local(0)
    {
    }
    /*! Неявный конструктор. */
    GlobalMsgId(const unsigned long &data) :
      local(0)
    {
        *reinterpret_cast<unsigned long*>(this) = data;
    }
    /*! Приведение типа к unsigned long */
    operator unsigned long&() {return *reinterpret_cast<unsigned long*>(this);}
    /*! Приведение типа из unsigned long */
    void operator =(const unsigned long &data) {*reinterpret_cast<unsigned long*>(this) = data;}
};

class ObjectInfo
{
public:
    typedef enum {Common, String} Type;

private:
    void *mReadPtr, *mWritePtr;
    size_t mReadSize, mWriteSize;
    Type mType;
    string mName;

//    friend class ObjnetNode;

public:
    ObjectInfo() :
        mReadPtr(0L), mWritePtr(0L), mReadSize(0), mWriteSize(0), mType(Common)
    {
    }

    ObjectInfo &bindVariableRO(string name, void *ptr, size_t size)
    {
        mReadPtr = ptr;
        mReadSize = size;
        mWritePtr = 0;
        mWriteSize = 0;
        mType = Common;
        mName = name;
        return *this;
    }

    ObjectInfo &bindVariableWO(string name, void *ptr, size_t size)
    {
        mReadPtr = 0;
        mReadSize = 0;
        mWritePtr = ptr;
        mWriteSize = size;
        mType = Common;
        mName = name;
        return *this;
    }

    ObjectInfo &bindVariableRW(string name, void *ptr, size_t size)
    {
        mReadPtr = mWritePtr = ptr;
        mReadSize = mWriteSize = size;
        mType = Common;
        mName = name;
        return *this;
    }

    ObjectInfo &bindVariableInOut(string name, void *inPtr, size_t inSize, void *outPtr, size_t outSize)
    {
        mReadPtr = outPtr;
        mReadSize = outSize;
        mWritePtr = inPtr;
        mWriteSize = inSize;
        mType = Common;
        mName = name;
        return *this;
    }

    ObjectInfo &bindString(string name, string *str)
    {
        mReadPtr = mWritePtr = str;
        mReadSize = mWriteSize = 1;
        mType = String;
        mName = name;
        return *this;
    }

    ByteArray read()
    {
        if (!mReadSize)
            return ByteArray();
        if (mType == String)
        {
            string *str = reinterpret_cast<string*>(mReadPtr);
            if (str)
                return ByteArray(str->c_str(), str->length());
            return ByteArray();
        }
        else
        {
            return ByteArray(reinterpret_cast<const char*>(mReadPtr), mReadSize);
        }
    }

    bool write(const ByteArray &ba)
    {
        if (!mWriteSize)
            return false;
        if (mType == String)
        {
            string *str = reinterpret_cast<string*>(mWritePtr);
            if (!str)
                return false;
            else
                *str = string(ba.data(), ba.size());
            return true;
        }
        else if ((size_t)ba.size() == mWriteSize)
        {
            for (size_t i=0; i<mWriteSize; i++)
                reinterpret_cast<unsigned char*>(mWritePtr)[i] = ba[i];
            return true;
        }
        return false;
    }
};

}

#endif
