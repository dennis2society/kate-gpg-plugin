/*
 * This file is part of kate-gpg-plugin (https://github.com/dennis2society).
 * Copyright (c) 2023 Dennis Luebke.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

/**
 * @brief This class contains the details for a GPG key
 **/

#include <QString>
#include <QVector>
#include <gpgme.h>
#include <gpgme++/key.h>

class GPGKeyDetails {
public:
  GPGKeyDetails();

  ~GPGKeyDetails();

  QString fingerPrint() const;
  QString keyType() const;
  QString keyLength() const;
  QString creationDate() const;
  QString expiryDate() const;
  const QVector<QString>& uids() const;   // this returns a list of all "IDs" per key
  const QVector<QString>& mailAdresses() const;   // this returns a list of all email addresses associated with this key

  size_t getNumUIds() const;

  void loadFromGPGMeKey(GpgME::Key key_);

private:
  QString m_fingerPrint;
  QString m_keyType;
  QString m_keyLength;
  QString m_creationDate;
  QString m_expiryDate;
  QVector<QString> m_uids;
  QVector<QString> m_mailAddresses;
};
