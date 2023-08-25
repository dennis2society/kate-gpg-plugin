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
#include <QTableWidget>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <memory>
#include <GPGMeWrapper.hpp>

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

  void onViewChanged(KTextEditor::View *v);

public slots:
  void setPreferredEmailAddress();  // use your own email address if you want to
                                    // encrypt to yourself
  void onTableViewSelection();    // listen to changes in the GPG key list table
  void onPreferredEmailAddressChanged(QString s_);
  void decryptButtonPressed();
  void encryptButtonPressed();

private:
  KTextEditor::MainWindow *m_mainWindow = nullptr;
  // The top level toolview widget
  std::unique_ptr<QWidget> m_toolview;

  GPGMeWrapper *m_gpgWrapper = nullptr;

  int m_selectedRowIndex;

  // A temporary textbox to display debug output
  QTextBrowser *m_debugTextBox = nullptr;

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
  QTableWidget *m_gpgKeyTable;
  QStringList m_gpgKeyTableHeader;
  QLabel *m_selectedKeyIndexLabel;

  // private functions
  void updateKeyTable();

  const QTableWidgetItem
  convertKeyDetailsToTableItem(const GPGKeyDetails &keyDetails_);

  void makeTableCell(const QString cellValue, uint row, uint col);

  void debugOutput(const QString& debugMessage, const QString& errorMessage);
};
