/*
 *
Copyright (C) 2016  Gabriele Salvato

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#ifndef CLIENTLISTDIALOG_H
#define CLIENTLISTDIALOG_H

#include <QObject>
#include <QDialog>
#include <QListWidget>

QT_FORWARD_DECLARE_CLASS(QGroupBox)
QT_FORWARD_DECLARE_CLASS(QListWidgetItem)
QT_FORWARD_DECLARE_CLASS(QSlider)
QT_FORWARD_DECLARE_CLASS(QComboBox)

class ClientListDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ClientListDialog(QWidget *parent);
    ~ClientListDialog();
    int exec();

public slots:

signals:

private slots:
    void onClientSelected(QListWidgetItem* selectedClient);

private:
    QGroupBox* createClientListBox();

private:
    QWidget           *pMyParent;
    QListWidget       clientListWidget;
    QPushButton       *closeButton;
    QString            sSelectedClient;

public:
    void clear();
    void addItem(QString sAddress);
};

#endif // CLIENTLISTDIALOG_H
