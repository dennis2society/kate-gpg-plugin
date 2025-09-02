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

#include <GPGMeWrapper.hpp>
#include <KTextEditor/Document>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Plugin>
#include <KTextEditor/View>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QObject>
#include <QPushButton>
#include <QSettings>
#include <QTableWidget>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <memory>

// forward declaration
class GPGKeyDetails;

class KateGPGPlugin : public KTextEditor::Plugin {
  Q_OBJECT
 public:
  explicit KateGPGPlugin(QObject *parent,
                         const QList<QVariant> & = QList<QVariant>())
      : KTextEditor::Plugin(parent) {}

  QObject *createView(KTextEditor::MainWindow *mainWindow) override;
};

class KateGPGPluginView : public QObject, public KXMLGUIClient {
  Q_OBJECT

 public:
  explicit KateGPGPluginView(KateGPGPlugin *plugin,
                             KTextEditor::MainWindow *mainwindow);

  ~KateGPGPluginView();

  void onViewChanged(KTextEditor::View *v);

 public slots:
  void setPreferredEmailAddress();  // use your own email address if you want to
                                    // encrypt to yourself
  void onTableViewSelection();  // listen to changes in the GPG key list table
  void onPreferredEmailAddressChanged(QString s_);
  void onShowOnlyPrivateKeysChanged();
  void onHideExpiredKeysChanged();
  void decryptButtonPressed();
  void encryptButtonPressed();

 private:
  KTextEditor::MainWindow *m_mainWindow = nullptr;
  // The top level toolview widget
  std::unique_ptr<QWidget> m_toolview;

  const QString m_settingsName = QString("kate_gpg_plugin_settings");

  GPGMeWrapper *m_gpgWrapper = nullptr;

  int m_selectedRowIndex;

  QPushButton *m_gpgDecryptButton = nullptr;
  QPushButton *m_gpgEncryptButton = nullptr;

  QVBoxLayout *m_verticalLayout;
  QLabel *m_titleLabel;
  QLabel *m_preferredEmailAddressLabel;
  QLabel *m_preferredGPGKeyIDLabel;
  QString m_title;
  QString m_preferredEmailAddress;
  QLineEdit *m_preferredEmailLineEdit;
  QString m_preferredGPGKeyID;
  QLabel *m_EmailAddressSelectLabel;
  QComboBox *m_preferredEmailAddressComboBox;
  QLineEdit *m_selectedKeyIndexEdit;
  QCheckBox *m_saveAsASCIICheckbox;
  QCheckBox *m_symmetricEncryptioCheckbox;
  QCheckBox *m_showOnlyPrivateKeysCheckbox;
  QCheckBox *m_hideExpiredKeysCheckbox;
  QTableWidget *m_gpgKeyTable;
  QStringList m_gpgKeyTableHeader;

  QSettings *m_pluginSettings;

  // private functions
  void updateKeyTable();

  const QTableWidgetItem convertKeyDetailsToTableItem(
      const GPGKeyDetails &keyDetails_);

  void makeTableCell(const QString cellValue, uint row, uint col);

  void readPluginSettings();
  void savePluginSettings();

  // Kate does not print debug output to commandline.
  // So we will simply abuse the document text and replace it
  // with relevant info for debug purposes!
  void setDebugTextInDocument(const QString &text_);

  // Functions to hook into Kate's save dialog
  // (used for auto-encryption on save)
  void connectToOpenAndSaveDialog(KTextEditor::Document *doc);
  void onDocumentWillSave(KTextEditor::Document *doc);
};
