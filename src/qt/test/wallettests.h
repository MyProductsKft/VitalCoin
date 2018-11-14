#ifndef VITALCOIN_QT_TEST_WALLETTESTS_H
#define VITALCOIN_QT_TEST_WALLETTESTS_H

#include <QObject>
#include <QTest>

class WalletTests : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void walletTests();
};

#endif // VITALCOIN_QT_TEST_WALLETTESTS_H
