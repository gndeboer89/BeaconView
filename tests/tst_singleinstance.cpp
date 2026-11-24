#include <QtTest>
#include "singleinstance.h"

class TstSingleInstance : public QObject {
    Q_OBJECT
private slots:
    void secondCopyIsNotPrimaryAndWakesTheFirst() {
        const QString key = "BeaconViewTest" + QString::number(QCoreApplication::applicationPid());
        SingleInstance first(key);
        QVERIFY(first.isPrimary());
        QSignalSpy spy(&first, &SingleInstance::activated);
        {
            SingleInstance second(key);                 // the same program started again
            QVERIFY(!second.isPrimary());
        }
        QVERIFY(spy.wait(2000) || spy.count() == 1);
        QCOMPARE(spy.count(), 1);
        {
            SingleInstance third(key);
            QVERIFY(!third.isPrimary());
        }
        QTRY_COMPARE(spy.count(), 2);
    }
    void anotherKeyIsIndependent() {
        const QString a = "BeaconViewTestA" + QString::number(QCoreApplication::applicationPid());
        SingleInstance one(a), other(a + "B");
        QVERIFY(one.isPrimary());
        QVERIFY(other.isPrimary());
    }
    void startsAgainAfterTheFirstQuit() {
        const QString key = "BeaconViewTestC" + QString::number(QCoreApplication::applicationPid());
        { SingleInstance first(key); QVERIFY(first.isPrimary()); }
        SingleInstance again(key);
        QVERIFY(again.isPrimary());
    }
};
QTEST_GUILESS_MAIN(TstSingleInstance)
#include "tst_singleinstance.moc"
