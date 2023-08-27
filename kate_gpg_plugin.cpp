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

#include <GPGKeyDetails.hpp>
#include <KLocalizedString>
#include <KPluginFactory>
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

KateGPGPluginView::KateGPGPluginView(KateGPGPlugin *plugin,
                                     KTextEditor::MainWindow *mainwindow)
    : m_mainWindow(mainwindow) {
  m_gpgWrapper = new GPGMeWrapper();
  m_toolview.reset(m_mainWindow->createToolView(
      plugin,                        // pointer to plugin
      "gpgPlugin",                   // just an identifier for the toolview
      KTextEditor::MainWindow::Left, // we want to create a toolview on the
                                     // left side
      QIcon::fromTheme("security-medium"),
      i18n("GPG Plugin"))); // User visible name of the toolview, i18n means it
                            // will be available for translation
  m_toolview->setMinimumHeight(700);

  // BUTTONS!
  m_gpgDecryptButton = new QPushButton("GPG DEcrypt current document");
  m_gpgEncryptButton = new QPushButton("GPG ENcrypt current document");

  // Connect the view changed signal to our slot
  // connect(m_mainWindow, &KTextEditor::MainWindow::viewChanged, this,
  //          &KateGPGPluginView::onViewChanged);

  // Lots of initialization and setting parameters for Qt UI stuff
  m_verticalLayout = new QVBoxLayout();
  //m_verticalLayout->setSizeConstraint(QLayout::SetMinimumSize);
  m_titleLabel = new QLabel("<b>Kate GPG Plugin Settings</b>");
  m_titleLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
  m_preferredEmailAddress = QString("");
  m_preferredGPGKeyID = QString("Key ID");
  m_preferredEmailAddressLabel = new QLabel(
      "A search string used to filter keys by available email addresses");
  m_preferredEmailAddressLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  m_preferredEmailLineEdit = new QLineEdit(m_preferredEmailAddress);
  m_preferredGPGKeyIDLabel =
      new QLabel("Selected GPG Key finerprint for encryption");
  m_EmailAddressSelectLabel =
      new QLabel("<b>To: Email address used for encryption</b>");
  m_preferredGPGKeyIDLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
  m_EmailAddressSelectLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);

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

  m_gpgKeyTable = new QTableWidget(m_gpgWrapper->getNumKeys(), 5);
  m_gpgKeyTable->setSelectionBehavior(QAbstractItemView::SelectRows);

  // we want the settings stuff in QScrollArea
  QScrollArea *scrollArea = new QScrollArea();
  scrollArea->setWidgetResizable(true);
  scrollArea->setMinimumHeight(700);
  scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  //scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  scrollArea->setAlignment(Qt::AlignTop);

  QWidget *w = new QWidget();
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
  m_verticalLayout->addWidget(m_gpgKeyTable);

  m_verticalLayout->insertStretch(-1, 1);

  connect(m_gpgKeyTable, SIGNAL(itemSelectionChanged()), this,
          SLOT(onTableViewSelection()));
  connect(m_preferredEmailLineEdit, SIGNAL(textChanged(QString)), this,
          SLOT(onPreferredEmailAddressChanged(QString)));
  connect(m_gpgDecryptButton, SIGNAL(released()), this,
          SLOT(decryptButtonPressed()));
  connect(m_gpgEncryptButton, SIGNAL(released()), this,
          SLOT(encryptButtonPressed()));

  updateKeyTable();
}

/// This is a left-over of the example project that creates a
/// Markdown preview.
/// I will leavie it here to remember where I have taken the
/// basic plugin structure from:
/// https://develop.kde.org/docs/apps/kate/plugin/
///
// void KateGPGPluginView::onViewChanged(KTextEditor::View *v) {
//   if (!v || !v->document()) {
//     return;
//   }

//  if (v->document()->highlightingMode().toLower() == "markdown") {
//    m_debugTextBox->setMarkdown(v->document()->text());
//  }
//}

void KateGPGPluginView::onPreferredEmailAddressChanged(QString s_) {
  m_preferredEmailAddress = m_preferredEmailLineEdit->text();
  updateKeyTable();
}

void KateGPGPluginView::decryptButtonPressed() {
  QList<KTextEditor::View *> views = m_mainWindow->views();
  if (views.size() < 1) {
    QMessageBox msg;
    msg.setText("Error!");
    msg.setInformativeText("No views available...");
    msg.setDefaultButton(QMessageBox::Ok);
    msg.exec();
    return;
  }
  KTextEditor::View *v = views.at(0);
  if (!v || !v->document()) {
    QMessageBox msg;
    msg.setText("Error Dycrypting Text!");
    msg.setInformativeText("Document is empty...");
    msg.setDefaultButton(QMessageBox::Ok);
    msg.exec();
    return;
  }
  if (m_selectedKeyIndexEdit->text().isEmpty()) {
    QMessageBox msg;
    msg.setText("Error Decrypting Text!");
    msg.setInformativeText("No fingerprint selected...");
    msg.setDefaultButton(QMessageBox::Ok);
    msg.exec();
    return;
  }
  GPGOperationResult res = m_gpgWrapper->decryptString(
      v->document()->text(), m_selectedKeyIndexEdit->text());
  if (!res.keyFound) {
    QMessageBox msg;
    msg.setText("Error Decrypting Text!");
    msg.setInformativeText("No matching fingerprint found...");
    msg.setDefaultButton(QMessageBox::Ok);
    msg.exec();
    return;
  }
  if (!res.decryptionSuccess) {
    QMessageBox msg;
    msg.setText("Error Decrypting Text!");
    msg.setInformativeText(res.errorMessage);
    msg.setDefaultButton(QMessageBox::Ok);
    msg.exec();
    return;
  }
  v->document()->setText(res.resultString);
}

void KateGPGPluginView::encryptButtonPressed() {
  QList<KTextEditor::View *> views = m_mainWindow->views();
  if (views.size() < 1) {
    QMessageBox msg;
    msg.setText("Error!");
    msg.setInformativeText("No views available...");
    msg.setDefaultButton(QMessageBox::Ok);
    msg.exec();
    return;
  }
  KTextEditor::View *v = views.at(0);

  if (!v || !v->document()) {
    QMessageBox msg;
    msg.setText("Error Encrypting Text!");
    msg.setInformativeText("No document available...");
    msg.setDefaultButton(QMessageBox::Ok);
    msg.exec();
    return;
  }
  if (v->document()->text().isEmpty()) {
    QMessageBox msg;
    msg.setText("Error Encrypting Text!");
    msg.setInformativeText("Document is empty...");
    msg.setDefaultButton(QMessageBox::Ok);
    msg.exec();
    return;
  }
  if (m_selectedKeyIndexEdit->text().isEmpty()) {
    QMessageBox msg;
    msg.setText("Error Decrypting Text!");
    msg.setInformativeText("No fingerprint selected...");
    msg.setDefaultButton(QMessageBox::Ok);
    msg.exec();
    return;
  }

  GPGOperationResult res = m_gpgWrapper->encryptString(
      v->document()->text(), m_selectedKeyIndexEdit->text(),
      m_preferredEmailAddressComboBox->itemText(
          m_preferredEmailAddressComboBox->currentIndex()),
      m_symmetricEncryptioCheckbox->isChecked());
  if (!res.keyFound) {
    QMessageBox msg;
    msg.setText("Error Encrypting Text!");
    msg.setInformativeText("No Matching Fingerprint found...\n"+res.errorMessage);
    msg.setDefaultButton(QMessageBox::Ok);
    msg.exec();
    return;
  }
  if (!res.decryptionSuccess) {
    QMessageBox msg;
    msg.setText("Error Encrypting Text!");
    msg.setInformativeText(res.errorMessage);
    msg.setDefaultButton(QMessageBox::Ok);
    msg.exec();
    return;
  }
  v->document()->text();
  v->document()->setText(res.resultString);
}

void KateGPGPluginView::onTableViewSelection() {
  /**
   * Thanks to sorting the table by creation date, we will here
   * search for the selected table row by key fingerprint in the
   * list of available GPG keys.
   */
  m_preferredEmailAddressComboBox->clear();
  m_gpgWrapper->loadKeys(m_preferredEmailLineEdit->text());
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

QString
concatenateEmailAddressesToString(const QVector<QString> uids_,
                                  const QVector<QString> mailAddresses_) {
  assert(uids_.size() == mailAddresses_.size());
  QString out = "";
  for (auto i = 0; i < mailAddresses_.size(); ++i) {
    out += uids_.at(i) + " <";
    out += mailAddresses_.at(i) + ">\n";
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
    QString uidsAndMails(
        concatenateEmailAddressesToString(d.uids(), d.mailAdresses()));
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
