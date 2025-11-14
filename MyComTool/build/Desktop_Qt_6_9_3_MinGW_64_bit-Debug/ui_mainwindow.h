/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.9.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_frmComTool
{
public:
    QVBoxLayout *verticalLayout;

    void setupUi(QWidget *frmComTool)
    {
        if (frmComTool->objectName().isEmpty())
            frmComTool->setObjectName("frmComTool");
        frmComTool->resize(800, 600);
        verticalLayout = new QVBoxLayout(frmComTool);
        verticalLayout->setSpacing(6);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(9, 9, 9, 9);

        retranslateUi(frmComTool);

        QMetaObject::connectSlotsByName(frmComTool);
    } // setupUi

    void retranslateUi(QWidget *frmComTool)
    {
        frmComTool->setWindowTitle(QCoreApplication::translate("frmComTool", "\344\270\262\345\217\243\346\233\262\347\272\277", nullptr));
    } // retranslateUi

};

namespace Ui {
    class frmComTool: public Ui_frmComTool {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
