// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef VITALCOIN_QT_VITALCOINADDRESSVALIDATOR_H
#define VITALCOIN_QT_VITALCOINADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class VitalcoinAddressEntryValidator : public QValidator {
  Q_OBJECT

public:
  explicit VitalcoinAddressEntryValidator(QObject *parent);

  State validate(QString &input, int &pos) const;
};

/** Vitalcoin address widget validator, checks for a valid vitalcoin address.
 */
class VitalcoinAddressCheckValidator : public QValidator {
  Q_OBJECT

public:
  explicit VitalcoinAddressCheckValidator(QObject *parent);

  State validate(QString &input, int &pos) const;
};

#endif // VITALCOIN_QT_VITALCOINADDRESSVALIDATOR_H
