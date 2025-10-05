/*
    SPDX-FileCopyrightText: 2025 Dennis Lübke <kde@dennis2society.de>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <qdebug.h>

#include <KConfigGroup>
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
#include <gpgkeydetails.hpp>
#include <kategpgplugin.hpp>

K_PLUGIN_FACTORY_WITH_JSON(KateGPGPluginFactory, "kategpgplugin.json",
                           registerPlugin<KateGPGPlugin>();)

QObject *KateGPGPlugin::createView(KTextEditor::MainWindow *mainWindow) {
  return new KateGPGPluginView(this, mainWindow);
}

KateGPGPluginView::~KateGPGPluginView() { savePluginSettings(); }

void KateGPGPluginView::readPluginSettings() {
  if (m_pluginConfig != nullptr) {
    KConfigGroup group = m_pluginConfig->group(m_pluginConfigGroupName);
    uint comboIndex = group.readEntry("selected_mail_address_index", 0);
    m_saveAsASCIICheckbox->setChecked(group.readEntry("use_ASCII_armor", true));
    m_symmetricEncryptioCheckbox->setChecked(
        group.readEntry("use_symmetric_encryption", false));
    m_showOnlyPrivateKeysCheckbox->setChecked(
        group.readEntry("show_only_private_keys", true));
    m_hideExpiredKeysCheckbox->setChecked(
        group.readEntry("hide_expired_secret_keys", true));
    m_preferredEmailLineEdit->setText(group.readEntry("search_string", ""));
    m_selectedRowIndex = group.readEntry("selected_key_index", 0);
    if (m_gpgKeyTable->rowCount() > 0) {
      m_gpgKeyTable->selectRow(m_selectedRowIndex);
    }
    uint numpreferredEmailAddressComboBoxCount =
        m_preferredEmailAddressComboBox->count();
    if (comboIndex <= numpreferredEmailAddressComboBoxCount) {
      m_preferredEmailAddressComboBox->setCurrentIndex(
          group.readEntry("selected_mail_address_index", 0));
    }
  }
}

void KateGPGPluginView::savePluginSettings() {
  if (m_pluginConfig != nullptr) {
    KConfigGroup group = m_pluginConfig->group(m_pluginConfigGroupName);
    group.writeEntry("search_string", m_preferredEmailLineEdit->text());
    group.writeEntry("selected_key_index", m_selectedRowIndex);
    group.writeEntry("selected_mail_address_index",
                     m_preferredEmailAddressComboBox->currentIndex());
    group.writeEntry("use_ASCII_armor", m_saveAsASCIICheckbox->isChecked());
    group.writeEntry("use_symmetric_encryption",
                     m_symmetricEncryptioCheckbox->isChecked());
    group.writeEntry("show_only_private_keys",
                     m_showOnlyPrivateKeysCheckbox->isChecked());
    group.writeEntry("hide_expired_secret_keys",
                     m_hideExpiredKeysCheckbox->isChecked());
    m_pluginConfig->sync();
  }
}

KateGPGPluginView::KateGPGPluginView(KateGPGPlugin *plugin,
                                     KTextEditor::MainWindow *mainwindow)
    : m_mainWindow(mainwindow) {
  m_gpgWrapper = new GPGMeWrapper();
  m_toolview.reset(m_mainWindow->createToolView(
      plugin,                          // pointer to plugin
      QString::fromUtf8("gpgPlugin"),  // just an identifier for the toolview
      KTextEditor::MainWindow::Left,   // we want to create a toolview on the
                                       // left side
      QIcon::fromTheme(QString::fromUtf8("security-medium")),
      i18n("GPG Plugin")));  // User visible name of the toolview, i18n means it
                             // will be available for translation
  m_toolview->setMinimumHeight(700);

  // BUTTONS!
  m_gpgDecryptButton = new QPushButton(i18n("GPG DEcrypt current document"));
  m_gpgEncryptButton = new QPushButton(i18n("GPG ENcrypt current document"));

  // Lots of initialization and setting parameters for Qt UI stuff
  m_verticalLayout = new QVBoxLayout(m_toolview.get());
  m_titleLabel = new QLabel(i18n("<b>GPG Plugin Settings</b>"));
  m_titleLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
  m_preferredEmailAddress = QString::fromUtf8("");
  m_preferredGPGKeyID = QString::fromUtf8("Key ID");
  m_preferredEmailAddressLabel = new QLabel(QString::fromUtf8(
      "A search string used to filter keys by available email addresses"));
  m_preferredEmailAddressLabel->setSizePolicy(QSizePolicy::Expanding,
                                              QSizePolicy::Fixed);
  m_preferredEmailLineEdit = new QLineEdit(m_preferredEmailAddress);
  m_preferredGPGKeyIDLabel = new QLabel(
      QString::fromUtf8("Selected GPG Key finerprint for encryption"));
  m_EmailAddressSelectLabel = new QLabel(
      QString::fromUtf8("<b>To: Email address used for encryption</b>"));
  m_preferredGPGKeyIDLabel->setSizePolicy(QSizePolicy::Maximum,
                                          QSizePolicy::Fixed);
  m_EmailAddressSelectLabel->setSizePolicy(QSizePolicy::Maximum,
                                           QSizePolicy::Fixed);

  m_preferredEmailAddressComboBox = new QComboBox();

  m_selectedKeyIndexEdit = new QLineEdit(m_preferredGPGKeyID);
  m_selectedKeyIndexEdit->setReadOnly(true);
  m_preferredEmailAddressComboBox->setToolTip(QString::fromUtf8(
      "Select an email address to which to encrypt.\n"
      "Only mail addresses associated with your currently selected\n"
      "key fingerprint are available."));
  m_selectedKeyIndexEdit->setToolTip(QString::fromUtf8(
      "This is your currently selected GPG key fingerprint that will be used "
      "for encryption."));

  m_saveAsASCIICheckbox =
      new QCheckBox(QString::fromUtf8("Save as ASCII encoded (.asc/.gpg)"));
  m_saveAsASCIICheckbox->setChecked(true);

  m_symmetricEncryptioCheckbox =
      new QCheckBox(QString::fromUtf8("Enable symmetric encryption"));
  m_symmetricEncryptioCheckbox->setChecked(false);

  m_showOnlyPrivateKeysCheckbox = new QCheckBox(
      QString::fromUtf8("Show only keys for which a private key is available"));
  m_showOnlyPrivateKeysCheckbox->setChecked(false);

  m_hideExpiredKeysCheckbox =
      new QCheckBox(QString::fromUtf8("Hide Expired Keys"));
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
  // m_pluginSettings = new QSettings(m_settingsName);
  m_pluginConfig = new KConfig(m_pluginConfigName);
  readPluginSettings();
}

void KateGPGPluginView::onPreferredEmailAddressChanged() {
  m_gpgKeyTable->itemSelectionChanged();
  m_preferredEmailAddress = m_preferredEmailLineEdit->text();
  updateKeyTable();
}

void KateGPGPluginView::onShowOnlyPrivateKeysChanged() {
  m_gpgKeyTable->itemSelectionChanged();
  m_preferredEmailAddress = m_preferredEmailLineEdit->text();
  updateKeyTable();
}

void KateGPGPluginView::onHideExpiredKeysChanged() {
  m_gpgKeyTable->itemSelectionChanged();
  m_preferredEmailAddress = m_preferredEmailLineEdit->text();
  updateKeyTable();
}

int pluginMessageBox(const QString msg_) {
  QMessageBox mb;
  mb.setText(QString::fromUtf8("GPG Plugin"));
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
  if ((doc->url().fileName().toLower().endsWith(QString::fromUtf8(".gpg")) ||
       doc->url().fileName().toLower().endsWith(QString::fromUtf8(".asc"))) &&
      m_gpgWrapper->isEncrypted(doc->text())) {
    decryptButtonPressed();
  }
}

void KateGPGPluginView::onDocumentWillSave(KTextEditor::Document *doc) {
  // Called right before save
  if (doc->url().fileName().toLower().endsWith(QString::fromUtf8(".gpg")) ||
      doc->url().fileName().toLower().endsWith(QString::fromUtf8(".asc"))) {
    QList<KTextEditor::View *> views = m_mainWindow->views();
    KTextEditor::View *v = views.at(0);
    if (m_gpgWrapper->isEncrypted(v->document()->text())) {
      pluginMessageBox(QString::fromUtf8(
          "Attempted double encryption detected!\nEncrypting more "
          "than once is disabled for now..."));
      return;
    }
    v->document()->setText(v->document()->text());
    encryptButtonPressed();
  }
}

void KateGPGPluginView::decryptButtonPressed() {
  QList<KTextEditor::View *> views = m_mainWindow->views();
  if (views.size() < 1) {
    pluginMessageBox(QString::fromUtf8("Error! No views available..."));
    return;
  }
  KTextEditor::View *v = views.at(0);
  if (!v || !v->document() || v->document()->isEmpty()) {
    pluginMessageBox(
        QString::fromUtf8("Error Decrypting Text! Document is empty.."));
    return;
  }
  if (m_selectedKeyIndexEdit->text().isEmpty()) {
    pluginMessageBox(
        QString::fromUtf8("Error Decrypting Text! No fingerprint selected..."));
    return;
  }
  GPGOperationResult res = m_gpgWrapper->decryptString(
      v->document()->text(), m_selectedKeyIndexEdit->text());
  if (!res.keyFound) {
    pluginMessageBox(
        QString::fromUtf8("Error Decrypting Text!ņ"
                          "No matching fingerprint found!\n"
                          "Or this is not a GPG "
                          "encrypted text..."));
    return;
  }
  if (!res.decryptionSuccess) {
    pluginMessageBox(QString::fromUtf8("Error Decrypting Text!\n") +
                     res.errorMessage);
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
    pluginMessageBox(QString::fromUtf8("Error! No views available..."));
    return;
  }
  KTextEditor::View *v = views.at(0);

  if (!v || !v->document()) {
    pluginMessageBox(
        QString::fromUtf8("Error Encrypting Text!\nNo document available..."));
    return;
  }
  if (v->document()->text().isEmpty()) {
    pluginMessageBox(
        QString::fromUtf8("Error Encrypting Text!\nDocument is empty.."));
    return;
  }
  if (m_selectedKeyIndexEdit->text().isEmpty()) {
    pluginMessageBox(QString::fromUtf8(
        "Error Encrypting Text!\nNo fingerprint selected..."));
    return;
  }
  if (v->document()->text().startsWith(
          QString::fromUtf8("-----BEGIN PGP MESSAGE-----"))) {
    pluginMessageBox(QString::fromUtf8(
        "Attempted double encryption detected!\nEncrypting twice "
        "is disabled for now..."));
    return;
  }

  GPGOperationResult res = m_gpgWrapper->encryptString(
      v->document()->text(), m_selectedKeyIndexEdit->text(),
      m_preferredEmailAddressComboBox->itemText(
          m_preferredEmailAddressComboBox->currentIndex()),
      m_saveAsASCIICheckbox->isChecked(),
      m_symmetricEncryptioCheckbox->isChecked());
  if (!res.keyFound) {
    pluginMessageBox(
        QString::fromUtf8(
            "Error Decrypting Text!\nNo Matching Fingerprint found...\n") +
        res.errorMessage);
    return;
  }
  if (!res.decryptionSuccess) {
    pluginMessageBox(QString::fromUtf8("Error Encrypting Text!") +
                     res.errorMessage);
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
  QString out = QString::fromUtf8("");
  for (auto i = 0; i < mailAddresses_.size(); ++i) {
    out += uids_.at(i) + QString::fromUtf8(" <");
    out += mailAddresses_.at(i) + QString::fromUtf8("> ");
    out += QString::fromUtf8("(") + subkeyIDs_.at(i) + QString::fromUtf8(")\n");
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
  m_gpgKeyTableHeader << QString::fromUtf8("Key Fingerprint")
                      << QString::fromUtf8("Creation Date")
                      << QString::fromUtf8("Expiry Date")
                      << QString::fromUtf8("Key Length")
                      << QString::fromUtf8("User IDs");
  m_gpgKeyTable->setHorizontalHeaderLabels(m_gpgKeyTableHeader);
  m_gpgKeyTable->resizeColumnsToContents();
  const QVector<GPGKeyDetails> &keyDetailsList = m_gpgWrapper->getKeys();
  uint numRows = 0;
  for (uint row = 0; row < m_gpgWrapper->getNumKeys(); ++row) {
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

#include "kategpgplugin.moc"
