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

void KateGPGPluginView::debugOutput(const QString &debugMessage,
                                    const QString &errorMessage) {
  if (!debugMessage.isEmpty()) {
    m_debugTextBox->setText(m_debugTextBox->toPlainText().prepend(
        "DEBUG:\n\n" + debugMessage + "\n\n"));
  }
  if (!errorMessage.isEmpty()) {
    m_debugTextBox->setText(m_debugTextBox->toPlainText().prepend(
        "ERROR:\n\n" + errorMessage + "\n\n"));
  }
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
      QIcon::fromTheme("preview"),
      i18n("GPG Plugin"))); // User visible name of the toolview, i18n means it
                            // will be available for translation

  // BUTTONS!
  m_gpgDecryptButton = new QPushButton("GPG DEcrypt current document");
  m_gpgEncryptButton = new QPushButton("GPG ENcrypt current document");

  // Connect the view changed signal to our slot
  // connect(m_mainWindow, &KTextEditor::MainWindow::viewChanged, this,
  //          &KateGPGPluginView::onViewChanged);

  // Lots of initialization and setting parameters for Qt UI stuff
  m_verticalLayout = new QVBoxLayout();
  m_titleLabel = new QLabel("<b>Kate GPG Plugin Settings</b>");
  m_preferredEmailAddress = QString("");
  m_preferredGPGKeyID = QString("Key ID");
  m_preferredEmailAddressLabel = new QLabel(
      "A search string used to filter keys by available email addresses");
  m_preferredEmailLineEdit = new QLineEdit(m_preferredEmailAddress);
  m_preferredGPGKeyIDLabel =
      new QLabel("Selected GPG Key finerprint for encryption");
  m_EmailAddressSelectLabel =
      new QLabel("<b>To: Email address used for encryption</b>");

  m_preferredEmailAddressComboBox = new QComboBox();

  m_preferredKeyIDEdit = new QLineEdit(m_preferredGPGKeyID);
  m_preferredKeyIDEdit->setEnabled(false);
  m_preferredEmailAddressComboBox->setToolTip(
      "Select an email address to which to encrypt.\n"
      "Only mail addresses associated with your currently selected\n"
      "key fingerprint are available.");
  m_preferredKeyIDEdit->setToolTip(
      "This is your currently selected GPG key fingerprint that will be used "
      "for encryption.");

  m_filterKeysCheckbox = new QCheckBox("Filter keys by search string");
  m_filterKeysCheckbox->setChecked(true);

  m_saveAsASCIICheckbox = new QCheckBox("Save as ASCII encoded (.asc/.gpg)");
  m_saveAsASCIICheckbox->setChecked(true);

  m_symmetricEncryptioCheckbox = new QCheckBox("Enable symmetric encryption");
  m_symmetricEncryptioCheckbox->setChecked(false);

  m_gpgKeyTable = new QTableWidget(m_gpgWrapper->getNumKeys(), 5);
  m_gpgKeyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_selectedKeyIndexLabel =
      new QLabel("Selected Key Index: " + QString::number(0) + "/" +
                 QString::number(m_gpgWrapper->getNumKeys()));
  m_selectedKeyIndexLabel->setSizePolicy(QSizePolicy::Minimum,
                                         QSizePolicy::Minimum);

  // we want the settings stuff in QScrollArea
  QScrollArea *scrollArea = new QScrollArea();
  scrollArea->setWidgetResizable(true);
  scrollArea->setMinimumHeight(1900);
  scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

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
  m_verticalLayout->addWidget(m_filterKeysCheckbox);
  m_verticalLayout->addWidget(m_EmailAddressSelectLabel);
  m_verticalLayout->addWidget(m_preferredEmailAddressComboBox);
  m_verticalLayout->addWidget(m_preferredGPGKeyIDLabel);
  m_verticalLayout->addWidget(m_preferredKeyIDEdit);
  m_verticalLayout->addWidget(m_gpgKeyTable);

  m_debugTextBox = new QTextBrowser(m_toolview.get());
  m_debugTextBox->setMinimumHeight(330);
  // m_debugTextBox->setMaximumHeight(900);
  m_debugTextBox->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  m_verticalLayout->addWidget(m_debugTextBox);

  connect(m_gpgKeyTable, SIGNAL(itemSelectionChanged()), this,
          SLOT(onTableViewSelection()));
  connect(m_filterKeysCheckbox, SIGNAL(stateChanged(int)), this,
          SLOT(onShowOnlyOwnMailsCheckboxChanged(int)));
  connect(m_preferredEmailLineEdit, SIGNAL(textChanged(QString)), this,
          SLOT(onPreferredEmailAddressChanged(QString)));
  connect(m_gpgDecryptButton, SIGNAL(released()), this,
          SLOT(decryptButtonPressed()));
  connect(m_gpgEncryptButton, SIGNAL(released()), this,
          SLOT(encryptButtonPressed()));

  updateKeyTable();
}

// void KateGPGPluginView::onViewChanged(KTextEditor::View *v) {
//   if (!v || !v->document()) {
//     return;
//   }

//  if (v->document()->highlightingMode().toLower() == "markdown") {
//    m_debugTextBox->setMarkdown(v->document()->text());
//  }
//}

void KateGPGPluginView::onShowOnlyOwnMailsCheckboxChanged(int i_) {
  updateKeyTable();
}

void KateGPGPluginView::onPreferredEmailAddressChanged(QString s_) {
  m_preferredEmailAddress = m_preferredEmailLineEdit->text();
  onShowOnlyOwnMailsCheckboxChanged(0);
}

void KateGPGPluginView::decryptButtonPressed() {
  QList<KTextEditor::View *> views = m_mainWindow->views();
  if (views.size() < 1) {
    debugOutput("", "No available views...");
    return;
  }
  debugOutput("NumViews: " + QString::number(views.size()), "");
  KTextEditor::View *v = views.at(0);
  debugOutput("DecrypButton pressed!", "");
  if (!v || !v->document()) {
    debugOutput("", "ERROR decrypting text! Document empty!");
    return;
  }
  debugOutput("Encrypted String:\n" + v->document()->text(), "");
  if (m_preferredKeyIDEdit->text().isEmpty()) {
    debugOutput("", "ERROR decrypting text! No fingerprint selected!");
    return;
  }
  GPGOperationResult res = m_gpgWrapper->decryptString(
      v->document()->text(), m_preferredKeyIDEdit->text());
  if (!res.keyFound) {
    debugOutput("",
                "Error decrypting text! No matching fingerprint found...\n" +
                    res.resultString);
    return;
  }
  if (!res.decryptionSuccess) {
    debugOutput("", "Error decrypting text! Decryption failed... Errorcode: " +
                        res.errorMessage);
    return;
  }
  debugOutput("Decrypted text: " + res.resultString, "");
  v->document()->setText(res.resultString);
}

void KateGPGPluginView::encryptButtonPressed() {
  QList<KTextEditor::View *> views = m_mainWindow->views();
  if (views.size() < 1) {
    debugOutput("", "No available views...");
    return;
  }
  debugOutput("NumViews: " + QString::number(views.size()), "");
  KTextEditor::View *v = views.at(0);

  if (!v || !v->document()) {
    debugOutput("", "ERROR encrypting text! Document empty!");
    return;
  }
  if (v->document()->text().isEmpty()) {
    debugOutput("", "ERROR encrypting text! No text in current document!");
    return;
  }
  if (m_preferredKeyIDEdit->text().isEmpty()) {
    debugOutput("", "ERROR encrypting text! No fingerprint selected!");
    return;
  }

  GPGOperationResult res = m_gpgWrapper->encryptString(
      v->document()->text(), m_preferredKeyIDEdit->text(),
      m_preferredEmailAddressComboBox->itemText(
          m_preferredEmailAddressComboBox->currentIndex()),
      m_symmetricEncryptioCheckbox->isChecked());
  if (!res.keyFound) {
    debugOutput("",
                "Error encrypting text! No matching fingerprint found...\n" +
                    res.resultString);
    return;
  }
  if (!res.decryptionSuccess) {
    debugOutput("", "Error encrypting text! Errorcode: " + res.errorMessage);
    return;
  }
  debugOutput("Encrypted text: " + res.resultString, "");
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
  QModelIndexList selectedList =
      m_gpgKeyTable->selectionModel()->selectedRows();
  // Currently it is possible to select multiple rows in the QTableWidget.
  // For now we will only consider the first selected row.
  if (selectedList.size() > 0) {
    m_selectedRowIndex = selectedList.at(0).row();
    const QString selectedFingerPrint(
        m_gpgKeyTable->item(m_selectedRowIndex, 0)->text());
    debugOutput("Selected Row: " + QString::number(m_selectedRowIndex), "");
    const QVector<GPGKeyDetails> keys = m_gpgWrapper->getKeys();
    if (m_selectedRowIndex <= keys.size()) {
      debugOutput("Selected Key Fingerprint: " + selectedFingerPrint, "");
      for (auto key = m_gpgWrapper->getKeys().begin();
           key != m_gpgWrapper->getKeys().end(); ++key) {
        GPGKeyDetails d = *key;
        if (selectedFingerPrint == d.fingerPrint()) {
          for (auto r = 0; r < d.mailAdresses().size(); ++r) {
            // QString uid = d.uids().at(r);
            QString mail = d.mailAdresses().at(r);
            m_preferredEmailAddressComboBox->addItem(mail);
          }
          m_preferredKeyIDEdit->setText(d.fingerPrint());
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
    bool showKey = false;
    if ((m_filterKeysCheckbox->isChecked()) &&
        (m_gpgWrapper->isPreferredKey(d, m_preferredEmailAddress))) {
      showKey = true;
    }
    if (!m_filterKeysCheckbox->isChecked()) {
      showKey = true;
    }
    if (showKey) {
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
  m_gpgKeyTable->setMinimumHeight(100);
  m_gpgKeyTable->setMaximumHeight(300);
  m_gpgKeyTable->resizeColumnsToContents();
  m_gpgKeyTable->resizeRowsToContents();
}

#include "kate_gpg_plugin.moc"
