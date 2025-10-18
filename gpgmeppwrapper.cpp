/*
    SPDX-FileCopyrightText: 2025 Dennis Lübke <kde@dennis2society.de>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <gpgme++/context.h>
#include <gpgme++/data.h>
#include <gpgme++/decryptionresult.h>
#include <gpgme++/encryptionresult.h>
#include <gpgme++/gpgmepp_version.h>
#include <gpgme++/key.h>
#include <gpgme++/keylistresult.h>

#include <KLocalizedString>
#include <gpgmeppwrapper.hpp>
#include <vector>

// This is needed to distinguish GPGMe++ versions
#define GPGMEPP_VERSION_NUMBER (GPGMEPP_VERSION_MAJOR * 10000 + GPGMEPP_VERSION_MINOR * 100 + GPGMEPP_VERSION_PATCH)

/// local functions
QVector<QString> getUIDsForKey(GpgME::Key key)
{
    QVector<QString> result;
    for (auto &uid : key.userIDs()) {
        result.append(QString::fromUtf8(uid.name()));
    }
    return result;
}

/// class functions
GPGMeWrapper::GPGMeWrapper()
{
    loadKeys(false, true, QString::fromUtf8(""));
}

GPGMeWrapper::~GPGMeWrapper()
{
    m_keys.clear();
}

uint GPGMeWrapper::selectedKeyIndex() const
{
    return m_selectedKeyIndex;
}

void GPGMeWrapper::setSelectedKeyIndex(uint newSelectedKeyIndex)
{
    m_selectedKeyIndex = newSelectedKeyIndex;
}

std::vector<GpgME::Key> GPGMeWrapper::listKeys(bool showOnlyPrivateKeys_, const QString &searchPattern_)
{
    GpgME::Error err;
    GpgME::Protocol protocol = GpgME::OpenPGP;
    GpgME::initializeLibrary();
    auto ctx = std::unique_ptr<GpgME::Context>(GpgME::Context::createForProtocol(protocol));
    unsigned int mode = 0;
    ctx->setKeyListMode(mode);
    std::vector<GpgME::Key> keys;
    err = ctx->startKeyListing(searchPattern_.toUtf8().constData(), showOnlyPrivateKeys_);
    if (err) {
        return keys;
    }
    while (true) {
        GpgME::Key key = ctx->nextKey(err);
        if (err.code()) {
            break;
        }
        keys.push_back(key);
    };
    return keys;
}

void GPGMeWrapper::loadKeys(bool showOnlyPrivateKeys_, bool hideExpiredKeys_, const QString searchPattern_)
{
    m_keys.clear();
    GPGOperationResult result;
    const std::vector<GpgME::Key> keys = listKeys(showOnlyPrivateKeys_, searchPattern_);
    if (keys.size() == 0) {
        result.errorMessage.append(i18n("Error! No keys found..."));
        return;
    }
    for (auto key = keys.begin(); key != keys.end(); ++key) {
        if (hideExpiredKeys_) {
            if (key->isExpired()) {
                continue;
            }
        }
        GPGKeyDetails d;
        d.loadFromGPGMeKey(*key);
        m_keys.push_back(d);
    }
    return;
}

const QVector<GPGKeyDetails> &GPGMeWrapper::getKeys() const
{
    return m_keys;
}

uint GPGMeWrapper::getNumKeys() const
{
    return m_keys.size();
}

bool GPGMeWrapper::isPreferredKey(const GPGKeyDetails d_, const QString &mailAddress_)
{
    for (auto &it : d_.mailAdresses()) {
        if (it.contains(mailAddress_)) {
            return true;
        }
    }
    return false;
}

const GPGOperationResult GPGMeWrapper::decryptString(const QString &inputString_, const QString &fingerprint_)
{
    GPGOperationResult result;
    GpgME::Error err;
    GpgME::Protocol protocol = GpgME::OpenPGP;
    unsigned int mode = 0;
    GpgME::initializeLibrary();
    auto ctx = std::unique_ptr<GpgME::Context>(GpgME::Context::createForProtocol(protocol));
    ctx->setArmor(true);
    ctx->setTextMode(true);
    ctx->setKeyListMode(mode);
    // find correct key
    const GpgME::Key key = ctx->key(fingerprint_.toUtf8().constData(), err, false);
    if (err) {
        result.errorMessage.append(i18n("Error finding key: ") + QString::fromStdString(err.asStdString()));
        return result;
    }
    result.keyFound = true;

    const QString::size_type length = inputString_.size();
    // To achieve non-volatile input for the GpgME++ decryption,
    // we have to transform the encrypted text to a const char* buffer
    // QString->toUtf8->constData()
    QByteArray bar = inputString_.toUtf8();
    GpgME::Data encryptedString(bar.constData(), length, true);
    GpgME::Data decryptedString;
    // attempt to decrypt
    GpgME::DecryptionResult d_res = ctx->decrypt(encryptedString, decryptedString);
// Meh... we have to distinguish GPGMe++ versions because Ubuntu ships
// 1.2.4 instead of >=2.0.0
#if GPGMEPP_VERSION_NUMBER < 20000
    if (!d_res.error()) {
#else
    if (!d_res.error().isError()) {
#endif
        result.decryptionSuccess = true;
        // result.keyIDUsedForDecryption = d_res.recipient(0).shortKeyID();
        for (uint i = 0; i < d_res.recipients().size(); ++i) {
            result.keyIDUsedForDecryption += QString::fromUtf8(d_res.recipients().at(i).keyID());
        }

    } else {
        result.errorMessage.append(d_res.error().asStdString());
        return result;
    }

    result.resultString = QString::fromStdString(decryptedString.toString());
    return result;
}

const GPGOperationResult GPGMeWrapper::encryptString(const QString &inputString_,
                                                     const QString &fingerprint_,
                                                     const QString &recipientMail_,
                                                     const bool useASCII,
                                                     bool symmetricEncryption_,
                                                     bool showOnlyPrivateKeys_)
{
    GPGOperationResult result;

    std::vector<GpgME::Key> selectedKeys;
    std::vector<GpgME::Key> keys = listKeys(showOnlyPrivateKeys_, recipientMail_);
    // find first key for selected fingerprint and mail address
    for (auto &key : keys) {
        const QString fingerprint = QString::fromUtf8(key.primaryFingerprint());
        if (fingerprint == fingerprint_) {
            result.keyFound = true;
            selectedKeys.push_back(key);
            break;
        }
    }

    GpgME::Error err;
    GpgME::Protocol protocol = GpgME::OpenPGP;
    GpgME::initializeLibrary();
    auto ctx = std::unique_ptr<GpgME::Context>(GpgME::Context::createForProtocol(protocol));
    ctx->setArmor(true);
    if (useASCII)
        ctx->setTextMode(true);

    QByteArray bar = inputString_.toUtf8();
    const qsizetype length = bar.length();
    GpgME::Data plainTextData = GpgME::Data(bar.constData(), length);
    GpgME::Data ciphertext;

    // encrypt
    // Using EncryptionFlags::NoEncryptTo returns a NotImplemented error... so we
    // have to use AlwaysTrust :/
    GpgME::Context::EncryptionFlags flags = GpgME::Context::EncryptionFlags::AlwaysTrust;
    if (symmetricEncryption_) {
        err = ctx->encryptSymmetrically(plainTextData, ciphertext);
        if (!err) {
            result.decryptionSuccess = true;
            result.resultString = QString::fromStdString(ciphertext.toString());
            return result;
        } else {
            result.resultString.append(i18n("ERROR in syymetric encryption: ") + QString::fromStdString(err.asStdString()));
            return result;
        }
    }
    GpgME::EncryptionResult enRes = ctx->encrypt(selectedKeys, plainTextData, ciphertext, flags);
// Meh... we have to distinguish GPGMe++ versions because Ubuntu ships
// 1.2.4 instead of >=2.0.0
#if GPGMEPP_VERSION_NUMBER < 20000
    if (!enRes.error()) {
#else
    if (!enRes.error().isError()) {
#endif
        result.decryptionSuccess = true;
        result.resultString = QString::fromStdString(ciphertext.toString());
        return result;
    } else {
        result.errorMessage.append(i18n("Encryption Failed: ") + QString::fromStdString(enRes.error().asStdString()));
        return result;
    }
    return result;
}

bool GPGMeWrapper::isEncrypted(const QString &inputString_)
{
    QByteArray bar = inputString_.toUtf8();
    GpgME::Data dataIn(bar.constData(), bar.size(),
                       false); // false = do not copy
    QByteArray outBuffer;
    GpgME::Data dataOut(outBuffer.constData(), outBuffer.size(), true);
    std::unique_ptr<GpgME::Context> ctx(GpgME::Context::createForProtocol(GpgME::OpenPGP));
    if (!ctx)
        return false;

    GpgME::DecryptionResult result = ctx->decrypt(dataIn, dataOut);

    return !result.error() && result.numRecipients() > 0;
}
