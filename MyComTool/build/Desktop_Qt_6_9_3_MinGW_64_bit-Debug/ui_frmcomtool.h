/********************************************************************************
** Form generated from reading UI file 'frmcomtool.ui'
**
** Created by: Qt User Interface Compiler version 6.9.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_FRMCOMTOOL_H
#define UI_FRMCOMTOOL_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_frmComTool
{
public:
    QVBoxLayout *verticalLayout;
    QWidget *widgetControl;
    QHBoxLayout *horizontalLayout;
    QLabel *labelPort;
    QComboBox *comboBoxPort;
    QLabel *labelBaud;
    QComboBox *comboBoxBaud;
    QPushButton *pushButtonOpen;
    QPushButton *pushButtonClear;
    QSpacerItem *horizontalSpacer;
    QSpacerItem *verticalSpacer;

    void setupUi(QWidget *frmComTool)
    {
        if (frmComTool->objectName().isEmpty())
            frmComTool->setObjectName("frmComTool");
        frmComTool->resize(1200, 800);
        verticalLayout = new QVBoxLayout(frmComTool);
        verticalLayout->setObjectName("verticalLayout");
        widgetControl = new QWidget(frmComTool);
        widgetControl->setObjectName("widgetControl");
        horizontalLayout = new QHBoxLayout(widgetControl);
        horizontalLayout->setObjectName("horizontalLayout");
        labelPort = new QLabel(widgetControl);
        labelPort->setObjectName("labelPort");
        labelPort->setStyleSheet(QString::fromUtf8("font-weight: bold;"));

        horizontalLayout->addWidget(labelPort);

        comboBoxPort = new QComboBox(widgetControl);
        comboBoxPort->setObjectName("comboBoxPort");

        horizontalLayout->addWidget(comboBoxPort);

        labelBaud = new QLabel(widgetControl);
        labelBaud->setObjectName("labelBaud");
        labelBaud->setStyleSheet(QString::fromUtf8("font-weight: bold;"));

        horizontalLayout->addWidget(labelBaud);

        comboBoxBaud = new QComboBox(widgetControl);
        comboBoxBaud->setObjectName("comboBoxBaud");

        horizontalLayout->addWidget(comboBoxBaud);

        pushButtonOpen = new QPushButton(widgetControl);
        pushButtonOpen->setObjectName("pushButtonOpen");
        pushButtonOpen->setStyleSheet(QString::fromUtf8("QPushButton {\n"
"    background-color: #2196F3;\n"
"    color: white;\n"
"    border: none;\n"
"    padding: 6px 12px;\n"
"    border-radius: 4px;\n"
"    font-weight: bold;\n"
"}\n"
"QPushButton:hover {\n"
"    background-color: #1976D2;\n"
"}\n"
"QPushButton:pressed {\n"
"    background-color: #0D47A1;\n"
"}"));

        horizontalLayout->addWidget(pushButtonOpen);

        pushButtonClear = new QPushButton(widgetControl);
        pushButtonClear->setObjectName("pushButtonClear");
        pushButtonClear->setStyleSheet(QString::fromUtf8("QPushButton {\n"
"    background-color: #f44336;\n"
"    color: white;\n"
"    border: none;\n"
"    padding: 6px 12px;\n"
"    border-radius: 4px;\n"
"    font-weight: bold;\n"
"}\n"
"QPushButton:hover {\n"
"    background-color: #d32f2f;\n"
"}\n"
"QPushButton:pressed {\n"
"    background-color: #b71c1c;\n"
"}"));

        horizontalLayout->addWidget(pushButtonClear);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);


        verticalLayout->addWidget(widgetControl);

        verticalSpacer = new QSpacerItem(20, 10, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout->addItem(verticalSpacer);


        retranslateUi(frmComTool);

        QMetaObject::connectSlotsByName(frmComTool);
    } // setupUi

    void retranslateUi(QWidget *frmComTool)
    {
        frmComTool->setWindowTitle(QCoreApplication::translate("frmComTool", "\345\267\245\344\270\232\347\233\221\346\216\247\347\263\273\347\273\237 - \346\270\251\345\272\246 & \347\224\265\345\216\213\345\256\236\346\227\266\347\233\221\346\265\213", nullptr));
        labelPort->setText(QCoreApplication::translate("frmComTool", "\344\270\262\345\217\243:", nullptr));
        labelBaud->setText(QCoreApplication::translate("frmComTool", "\346\263\242\347\211\271\347\216\207:", nullptr));
        pushButtonOpen->setText(QCoreApplication::translate("frmComTool", "\346\211\223\345\274\200\344\270\262\345\217\243", nullptr));
        pushButtonClear->setText(QCoreApplication::translate("frmComTool", "\346\270\205\347\251\272\346\225\260\346\215\256", nullptr));
    } // retranslateUi

};

namespace Ui {
    class frmComTool: public Ui_frmComTool {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_FRMCOMTOOL_H
