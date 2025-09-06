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

#include <qdebug.h>

#include <GPGKeyDetails.hpp>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KTextEditor/Application>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <QLayout>
#include <QMessageBox>
#include <QScrollArea>
#include <QScrollBar>
#include <QTableWidgetItem>
#include <kate_gpg_plugin.hpp>

K_PLUGIN_FACTORY_WITH_JSON(KateGPGPluginFactory, "kate_gpg_plugin.json",
                           registerPlugin<KateGPGPlugin>();)

QObject *KateGPGPlugin::createView(KTextEditor::MainWindow *mainWindow) {
  return new KateGPGPluginView(this, mainWindow);
}

KateGPGPluginView::~KateGPGPluginView() { savePluginSettings(); }

void KateGPGPluginView::readPluginSettings() {
  if (m_pluginSettings != nullptr) {
    m_pluginSettings->beginGroup("default");
    uint comboIndex =
        m_pluginSettings->value("selected_mail_address_index").toUInt();
    m_saveAsASCIICheckbox->setChecked(
        m_pluginSettings->value("use_ASCII_armor").toBool());
    m_symmetricEncryptioCheckbox->setChecked(
        m_pluginSettings->value("use_symmetric_encryption").toBool());
    m_showOnlyPrivateKeysCheckbox->setChecked(
        m_pluginSettings->value("show_only_private_keys").toBool());
    m_hideExpiredKeysCheckbox->setChecked(
        m_pluginSettings->value("hide_expired_secret_keys").toBool());
    m_preferredEmailLineEdit->setText(
        m_pluginSettings->value("search_string").toString());
    m_selectedRowIndex = m_pluginSettings->value("selected_key_index").toUInt();
    if (m_gpgKeyTable->rowCount() > 0) {
      m_gpgKeyTable->selectRow(m_selectedRowIndex);
    }
    if (comboIndex <= m_preferredEmailAddressComboBox->count()) {
      m_preferredEmailAddressComboBox->setCurrentIndex(
          m_pluginSettings->value("selected_mail_address_index").toUInt());
    }
    m_pluginSettings->endGroup();
  }
}

void KateGPGPluginView::savePluginSettings() {
  if (m_pluginSettings != nullptr) {
    m_pluginSettings->beginGroup("default");
    m_pluginSettings->setValue("search_string",
                               m_preferredEmailLineEdit->text());
    m_pluginSettings->setValue("selected_key_index", m_selectedRowIndex);
    m_pluginSettings->setValue("selected_mail_address_index",
                               m_preferredEmailAddressComboBox->currentIndex());
    m_pluginSettings->setValue("use_ASCII_armor",
                               m_saveAsASCIICheckbox->isChecked());
    m_pluginSettings->setValue("use_symmetric_encryption",
                               m_symmetricEncryptioCheckbox->isChecked());
    m_pluginSettings->setValue("show_only_private_keys",
                               m_showOnlyPrivateKeysCheckbox->isChecked());
    m_pluginSettings->setValue("hide_expired_secret_keys",
                               m_hideExpiredKeysCheckbox->isChecked());
    m_pluginSettings->endGroup();
  }
}

KateGPGPluginView::KateGPGPluginView(KateGPGPlugin *plugin,
                                     KTextEditor::MainWindow *mainwindow)
    : m_mainWindow(mainwindow) {
  m_gpgWrapper = new GPGMeWrapper();
  m_toolview.reset(m_mainWindow->createToolView(
      plugin,                         // pointer to plugin
      "gpgPlugin",                    // just an identifier for the toolview
      KTextEditor::MainWindow::Left,  // we want to create a toolview on the
                                      // left side
      QIcon::fromTheme("security-medium"),
      i18n("GPG Plugin")));  // User visible name of the toolview, i18n means it
                             // will be available for translation
  m_toolview->setMinimumHeight(700);

  // BUTTONS!
  m_gpgDecryptButton = new QPushButton("GPG DEcrypt current document");
  m_gpgEncryptButton = new QPushButton("GPG ENcrypt current document");

  // Lots of initialization and setting parameters for Qt UI stuff
  m_verticalLayout = new QVBoxLayout(m_toolview.get());
  m_titleLabel = new QLabel("<b>Kate GPG Plugin Settings</b>");
  m_titleLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
  m_preferredEmailAddress = QString("");
  m_preferredGPGKeyID = QString("Key ID");
  m_preferredEmailAddressLabel = new QLabel(
      "A search string used to filter keys by available email addresses");
  m_preferredEmailAddressLabel->setSizePolicy(QSizePolicy::Expanding,
                                              QSizePolicy::Fixed);
  m_preferredEmailLineEdit = new QLineEdit(m_preferredEmailAddress);
  m_preferredGPGKeyIDLabel =
      new QLabel("Selected GPG Key finerprint for encryption");
  m_EmailAddressSelectLabel =
      new QLabel("<b>To: Email address used for encryption</b>");
  m_preferredGPGKeyIDLabel->setSizePolicy(QSizePolicy::Maximum,
                                          QSizePolicy::Fixed);
  m_EmailAddressSelectLabel->setSizePolicy(QSizePolicy::Maximum,
                                           QSizePolicy::Fixed);

  m_preferredEmailAddressComboBox = new QComboBox();

  m_selectedKeyIndexEdit = new QLineEdit(m_preferredGPGKeyID);
  m_selectedKeyIndexEdit->setReadOnly(true);
  m_preferredEmailAddressComboBox->setToolTip(
      "Select an email address to which to encrypt.\n"
      "Only mail addresses associated with your currently selected\n"
      "key fingerprint are available.");
  m_selectedKeyIndexEdit->setToolTip(
      "This is your currently selected GPG key fingerprint that will be used "
      "for encryption.");

  m_saveAsASCIICheckbox = new QCheckBox("Save as ASCII encoded (.asc/.gpg)");
  m_saveAsASCIICheckbox->setChecked(true);

  m_symmetricEncryptioCheckbox = new QCheckBox("Enable symmetric encryption");
  m_symmetricEncryptioCheckbox->setChecked(false);

  m_showOnlyPrivateKeysCheckbox =
      new QCheckBox("Show only keys for which a private key is available");
  m_showOnlyPrivateKeysCheckbox->setChecked(false);

  m_hideExpiredKeysCheckbox = new QCheckBox("Hide Expired Keys");
  m_hideExpiredKeysCheckbox->setChecked(true);

  m_gpgKeyTable =
      new QTableWidget(m_gpgWrapper->getNumKeys(), 5, m_toolview.get());
  m_gpgKeyTable->setSelectionBehavior(QAbstractItemView::SelectRows);

  // we want the settings stuff in QScrollArea
  QScrollArea *scrollArea = new QScrollArea(m_toolview.get());
  scrollArea->setWidgetResizable(true);
  scrollArea->setMinimumHeight(700);
  scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  scrollArea->setAlignment(Qt::AlignTop);

  QWidget *w = new QWidget(m_toolview.get());
  w->setLayout(m_verticalLayout);
  scrollArea->setWidget(w);
  m_toolview->layout()->addWidget(scrollArea);
  m_verticalLayout->addWidget(m_titleLabel);
  m_verticalLayout->addWidget(m_gpgDecryptButton);
  m_verticalLayout->addWidget(m_gpgEncryptButton);
  m_verticalLayout->addWidget(m_saveAsASCIICheckbox);
  m_verticalLayout->addWidget(m_symmetricEncryptioCheckbox);
  m_verticalLayout->addWidget(m_preferredEmailAddressLabel);
  m_verticalLayout->addWidget(m_preferredEmailLineEdit);
  m_verticalLayout->addWidget(m_EmailAddressSelectLabel);
  m_verticalLayout->addWidget(m_preferredEmailAddressComboBox);
  m_verticalLayout->addWidget(m_preferredGPGKeyIDLabel);
  m_verticalLayout->addWidget(m_selectedKeyIndexEdit);
  m_verticalLayout->addWidget(m_showOnlyPrivateKeysCheckbox);
  m_verticalLayout->addWidget(m_hideExpiredKeysCheckbox);
  m_verticalLayout->addWidget(m_gpgKeyTable);

  m_verticalLayout->insertStretch(-1, 1);

  connect(m_gpgKeyTable, SIGNAL(itemSelectionChanged()), this,
          SLOT(onTableViewSelection()));
  connect(m_preferredEmailLineEdit, SIGNAL(textChanged(QString)), this,
          SLOT(onPreferredEmailAddressChanged(QString)));
  connect(m_showOnlyPrivateKeysCheckbox, SIGNAL(stateChanged(int)), this,
          SLOT(onShowOnlyPrivateKeysChanged()));
  connect(m_hideExpiredKeysCheckbox, SIGNAL(stateChanged(int)), this,
          SLOT(onHideExpiredKeysChanged()));
  connect(m_gpgDecryptButton, SIGNAL(released()), this,
          SLOT(decryptButtonPressed()));
  connect(m_gpgEncryptButton, SIGNAL(released()), this,
          SLOT(encryptButtonPressed()));
  // hook into open/save dialog
  connect(mainwindow, &KTextEditor::MainWindow::viewCreated, this,
          [this](KTextEditor::View *view) {
            connectToOpenAndSaveDialog(view->document());
          });
  updateKeyTable();

  // restore plugin settings
  m_pluginSettings = new QSettings(m_settingsName);
  readPluginSettings();
  qDebug() << "GPGMeVersion: " << m_gpgWrapper->getGpgMeVersion();
}

void KateGPGPluginView::onPreferredEmailAddressChanged(QString s_) {
  emit m_gpgKeyTable->itemSelectionChanged();
  m_preferredEmailAddress = m_preferredEmailLineEdit->text();
  updateKeyTable();
}

void KateGPGPluginView::onShowOnlyPrivateKeysChanged() {
  emit m_gpgKeyTable->itemSelectionChanged();
  m_preferredEmailAddress = m_preferredEmailLineEdit->text();
  updateKeyTable();
}

void KateGPGPluginView::onHideExpiredKeysChanged() {
  emit m_gpgKeyTable->itemSelectionChanged();
  m_preferredEmailAddress = m_preferredEmailLineEdit->text();
  updateKeyTable();
}

int pluginMessageBox(const QString msg_) {
  QMessageBox mb;
  mb.setText("GPG Plugin");
  mb.setInformativeText(msg_);
  mb.setDefaultButton(QMessageBox::Ok);
  return mb.exec();
}

void KateGPGPluginView::connectToOpenAndSaveDialog(KTextEditor::Document *doc) {
  connect(doc, &KTextEditor::Document::aboutToSave, this,
          &KateGPGPluginView::onDocumentWillSave);
  onDocumentOpened(doc);
}

void KateGPGPluginView::onDocumentOpened(KTextEditor::Document *doc) {
  if ((doc->url().fileName().toLower().endsWith(".gpg") ||
       doc->url().fileName().endsWith(".asc")) &&
      m_gpgWrapper->isEncrypted(doc->text())) {
    decryptButtonPressed();
  }
}

void KateGPGPluginView::onDocumentWillSave(KTextEditor::Document *doc) {
  // Called right before save
  if (doc->url().fileName().toLower().endsWith(".gpg") ||
      doc->url().fileName().endsWith(".asc")) {
    QList<KTextEditor::View *> views = m_mainWindow->views();
    KTextEditor::View *v = views.at(0);
    if (m_gpgWrapper->isEncrypted(v->document()->text())) {
      pluginMessageBox(
          "Attempted double encryption detected!\nEncrypting more "
          "than once is disabled for now...");
      return;
    }
    v->document()->setText(v->document()->text());
    encryptButtonPressed();
  }
}

void KateGPGPluginView::setDebugTextInDocument(const QString &text_) {
  QList<KTextEditor::View *> views = m_mainWindow->views();
  if (views.size() < 1) {
    pluginMessageBox("Error! No views available...");
    return;
  }
  KTextEditor::View *v = views.at(0);
  v->document()->setText(text_);
}

void KateGPGPluginView::decryptButtonPressed() {
  QList<KTextEditor::View *> views = m_mainWindow->views();
  if (views.size() < 1) {
    pluginMessageBox("Error! No views available...");
    return;
  }
  KTextEditor::View *v = views.at(0);
  if (!v || !v->document() || v->document()->isEmpty()) {
    pluginMessageBox("Error Decrypting Text! Document is empty..");
    return;
  }
  if (m_selectedKeyIndexEdit->text().isEmpty()) {
    pluginMessageBox("Error Decrypting Text! No fingerprint selected...");
    return;
  }
  GPGOperationResult res = m_gpgWrapper->decryptString(
      v->document()->text(), m_selectedKeyIndexEdit->text());
  if (!res.keyFound) {
    pluginMessageBox(
        "Error Decrypting Text!ņ"
        "No matching fingerprint found!\n"
        "Or this is not a GPG "
        "encrypted text...");
    return;
  }
  if (!res.decryptionSuccess) {
    pluginMessageBox("Error Decrypting Text!\n" + res.errorMessage);
    return;
  }
  v->document()->setText(res.resultString);
  // Search for decryption key ID in available keys
  // and autoselect corresponding row upon finding the correct one.
  for (auto i = 0; i < m_gpgKeyTable->rowCount(); ++i) {
    QTableWidgetItem *detailsItem = m_gpgKeyTable->item(i, 4);
    QString detailsString = detailsItem->text();
    if (detailsString.contains(res.keyIDUsedForDecryption)) {
      m_selectedRowIndex = i;
      m_gpgKeyTable->selectRow(i);
      break;
    }
  }
}

void KateGPGPluginView::encryptButtonPressed() {
  QList<KTextEditor::View *> views = m_mainWindow->views();
  if (views.size() < 1) {
    pluginMessageBox("Error! No views available...");
    return;
  }
  KTextEditor::View *v = views.at(0);

  if (!v || !v->document()) {
    pluginMessageBox("Error Encrypting Text!\nNo document available...");
    return;
  }
  if (v->document()->text().isEmpty()) {
    pluginMessageBox("Error Encrypting Text!\nDocument is empty..");
    return;
  }
  if (m_selectedKeyIndexEdit->text().isEmpty()) {
    pluginMessageBox("Error Encrypting Text!\nNo fingerprint selected...");
    return;
  }
  if (v->document()->text().startsWith("-----BEGIN PGP MESSAGE-----")) {
    pluginMessageBox(
        "Attempted double encryption detected!\nEncrypting twice "
        "is disabled for now...");
    return;
  }

  GPGOperationResult res = m_gpgWrapper->encryptString(
      v->document()->text(), m_selectedKeyIndexEdit->text(),
      m_preferredEmailAddressComboBox->itemText(
          m_preferredEmailAddressComboBox->currentIndex()),
      m_symmetricEncryptioCheckbox->isChecked());
  if (!res.keyFound) {
    pluginMessageBox(
        "Error Decrypting Text!\nNo Matching Fingerprint found...\n" +
        res.errorMessage);
    return;
  }
  if (!res.decryptionSuccess) {
    pluginMessageBox("Error Encrypting Text!" + res.errorMessage);
    return;
  }
  v->document()->setText(res.resultString);
}

void KateGPGPluginView::onTableViewSelection() {
  /**
   * Thanks to sorting the table by creation date, we will here
   * search for the selected table row by key fingerprint in the
   * list of available GPG keys.
   */
  m_preferredEmailAddressComboBox->clear();
  m_gpgWrapper->loadKeys(m_showOnlyPrivateKeysCheckbox->isChecked(),
                         m_hideExpiredKeysCheckbox->isChecked(),
                         m_preferredEmailLineEdit->text());
  QModelIndexList selectedList =
      m_gpgKeyTable->selectionModel()->selectedRows();
  // Currently it is possible to select multiple rows in the QTableWidget.
  // For now we will only consider the first selected row.
  if (selectedList.size() > 0) {
    m_selectedRowIndex = selectedList.at(0).row();
    const QString selectedFingerPrint(
        m_gpgKeyTable->item(m_selectedRowIndex, 0)->text());
    const QVector<GPGKeyDetails> keys = m_gpgWrapper->getKeys();
    if (m_selectedRowIndex <= keys.size()) {
      for (auto key = m_gpgWrapper->getKeys().begin();
           key != m_gpgWrapper->getKeys().end(); ++key) {
        GPGKeyDetails d = *key;
        if (selectedFingerPrint == d.fingerPrint()) {
          const QVector<QString> mailAddresses = d.mailAdresses();
          for (auto &r : mailAddresses) {
            m_preferredEmailAddressComboBox->addItem(r);
          }
          m_selectedKeyIndexEdit->setText(d.fingerPrint());
          return;
        }
      }
    }
  }
}

QString concatenateEmailAddressesToString(const QVector<QString> uids_,
                                          const QVector<QString> mailAddresses_,
                                          const QVector<QString> subkeyIDs_) {
  assert(uids_.size() == mailAddresses_.size());
  QString out = "";
  for (auto i = 0; i < mailAddresses_.size(); ++i) {
    out += uids_.at(i) + " <";
    out += mailAddresses_.at(i) + "> ";
    out += "(" + subkeyIDs_.at(i) + ")\n";
  }
  return out;
}

void KateGPGPluginView::makeTableCell(const QString cellValue, uint row,
                                      uint col) {
  QTableWidgetItem *item = new QTableWidgetItem(cellValue);
  m_gpgKeyTable->setItem(row, col, item);
}

void KateGPGPluginView::updateKeyTable() {
  m_gpgKeyTable->setSortingEnabled(false);
  m_gpgKeyTable->setRowCount(0);
  m_gpgKeyTableHeader << "Key Fingerprint"
                      << "Creation Date"
                      << "Expiry Date"
                      << "Key Length"
                      << "User IDs";
  m_gpgKeyTable->setHorizontalHeaderLabels(m_gpgKeyTableHeader);
  m_gpgKeyTable->resizeColumnsToContents();
  const QVector<GPGKeyDetails> &keyDetailsList = m_gpgWrapper->getKeys();
  uint numRows = 0;
  for (auto row = 0; row < m_gpgWrapper->getNumKeys(); ++row) {
    GPGKeyDetails d = keyDetailsList.at(row);
    m_gpgKeyTable->insertRow(m_gpgKeyTable->rowCount());
    makeTableCell(d.fingerPrint(), numRows, 0);
    makeTableCell(d.creationDate(), numRows, 1);
    makeTableCell(d.expiryDate(), numRows, 2);
    makeTableCell(d.keyLength(), numRows, 3);
    QString uidsAndMails(concatenateEmailAddressesToString(
        d.uids(), d.mailAdresses(), d.subkeyIDs()));
    makeTableCell(uidsAndMails, numRows, 4);
    ++numRows;
  }
  m_gpgKeyTable->resizeRowsToContents();
  m_gpgKeyTable->setSelectionMode(QAbstractItemView::ContiguousSelection);
  m_gpgKeyTable->sortByColumn(1, Qt::DescendingOrder);
  m_gpgKeyTable->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  m_gpgKeyTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  m_gpgKeyTable->setSortingEnabled(true);
  if (m_gpgKeyTable->rowCount() > 0) {
    m_gpgKeyTable->selectRow(0);
  }
  m_gpgKeyTable->setMinimumHeight(250);
  m_gpgKeyTable->setMaximumHeight(500);
  m_gpgKeyTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  m_gpgKeyTable->resizeColumnsToContents();
  m_gpgKeyTable->resizeRowsToContents();
}

#include "kate_gpg_plugin.moc"
