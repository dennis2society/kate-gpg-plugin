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

#include <QDateTime>
#include <string>
#include <vector>
#include <GPGKeyDetails.hpp>

GPGKeyDetails::GPGKeyDetails() {}

GPGKeyDetails::~GPGKeyDetails() {
  m_uids.clear();
  m_mailAddresses.clear();
  m_subkeyIDs.clear();
}

QString GPGKeyDetails::fingerPrint() const { return m_fingerPrint; }

QString GPGKeyDetails::keyID() const { return m_keyID; }

QString GPGKeyDetails::keyType() const { return m_keyType; }

QString GPGKeyDetails::keyLength() const { return m_keyLength; }

QString GPGKeyDetails::creationDate() const { return m_creationDate; }

QString GPGKeyDetails::expiryDate() const { return m_expiryDate; }

const QVector<QString>& GPGKeyDetails::uids() const { return m_uids; }

const QVector<QString>& GPGKeyDetails::mailAdresses() const { return m_mailAddresses; }

const QVector<QString>& GPGKeyDetails::subkeyIDs() const { return m_subkeyIDs; }

size_t GPGKeyDetails::getNumUIds() const { return m_uids.size(); }

const QString timestampToQString(const time_t timestamp_) {
  QDateTime dt;
  dt.setSecsSinceEpoch(timestamp_);
  return dt.toString(QString("yyyy-MM-dd"));
}

void GPGKeyDetails::loadFromGPGMeKey(GpgME::Key key_) {
  m_fingerPrint = QString(key_.primaryFingerprint());
  m_keyID = QString(key_.shortKeyID());
  m_keyType = QString::fromStdString(key_.subkey(0).algoName());
  m_keyLength = QString::number(key_.subkey(0).length());
  m_creationDate = QString(timestampToQString(key_.subkey(0).creationTime()));
  m_expiryDate = QString(timestampToQString(key_.subkey(0).expirationTime()));
  const std::vector<GpgME::UserID>& ids = key_.userIDs();
  for (auto &id : ids) {
      m_uids.push_back(id.name());
      m_mailAddresses.push_back(id.email());
      m_subkeyIDs.push_back(key_.subkey(1).keyID());
  }

}
