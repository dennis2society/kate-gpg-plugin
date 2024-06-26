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
 * @brief This is a convenience wrapper for GPGMepp/GpgMe++ calls.
 * We want returned datatypes in C++/Qt types+containers.
 */

#include <QVector>
#include <GPGKeyDetails.hpp>
#include <gpgme++/key.h>

struct GPGOperationResult {
  QString resultString;  // de- or encrypted string depending on operation
  bool keyFound = false;
  bool decryptionSuccess = false;
  QString errorMessage;
  QString keyIDUsedForDecryption;
};

class GPGMeWrapper {
private:
  // The list of available GPG Keys
  QVector<GPGKeyDetails> m_keys;

  // for convenience reasons we want to know the currently selected key from the
  // UI
  uint m_selectedKeyIndex;

  /**
   * @brief Gets all available GPG keys containing mail addresses
   *        with search pattern.
   * @param searchPattern_ The mail search pattern.
   * @return A list of matching GpgMe::Key
   */
  std::vector<GpgME::Key> listKeys(bool showOnlyPrivateKeys_, const QString &searchPattern_ = "");

public:
  GPGMeWrapper();

  ~GPGMeWrapper();

  const QVector<GPGKeyDetails> &getKeys() const;

  size_t getNumKeys() const;

  /**
   * @brief This function reads all available keys and
   *        adds its details to the keys list.
   */
  void loadKeys(bool showOnlyPrivateKeys_, bool hideExpiredKeys_, const QString searchPattern_);

  /**
   * @brief This function attempts to decrypt a given input string
   *        using any of the available private keys. Will fail if the
   *        message was not encrypted to your private key.
   * @param inputString_ The encrypted input string.
   * @param fingerprint_ This is used for planning ahead when attempting
   *                     to decrypt a message encrypted to multiple
   *                     recipients.
   * @return The GPGOerationsResult (see above)
   */
  const GPGOperationResult decryptString(const QString &inputString_,
                                         const QString &fingerprint_);

  const GPGOperationResult encryptString(const QString &inputString_,
                                         const QString &fingerprint_,
                                         const QString &recipientMail_,
                                         bool symmetricEncryption_ = false,
                                         bool showOnlyPrivateKeys_ = false);

  bool isPreferredKey(const GPGKeyDetails d_, const QString &mailAddress_);

  void setSelectedKeyIndex(uint newSelectedKeyIndex);
  uint selectedKeyIndex() const;
};
