/********************************************************************************
** Form generated from reading UI file 'layerDialog.ui'
**
** Created by: Qt User Interface Compiler version 5.2.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LAYERDIALOG_H
#define UI_LAYERDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_layerDialog
{
public:
    QWidget *dockWidgetContents;
    QVBoxLayout *verticalLayout;
    QTreeWidget *frameTreeWidget;
    QVBoxLayout *rasterLayout;
    QTreeWidget *rasterTreeWidget;
    QVBoxLayout *subLayout;
    QTreeWidget *decParsTree;
    QPlainTextEdit *logPlainTextEdit;

    void setupUi(QDockWidget *layerDialog)
    {
        if (layerDialog->objectName().isEmpty())
            layerDialog->setObjectName(QStringLiteral("layerDialog"));
        layerDialog->resize(100, 500);
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(layerDialog->sizePolicy().hasHeightForWidth());
        layerDialog->setSizePolicy(sizePolicy);
        layerDialog->setMinimumSize(QSize(100, 325));
        dockWidgetContents = new QWidget();
        dockWidgetContents->setObjectName(QStringLiteral("dockWidgetContents"));
        QSizePolicy sizePolicy1(QSizePolicy::Fixed, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(dockWidgetContents->sizePolicy().hasHeightForWidth());
        dockWidgetContents->setSizePolicy(sizePolicy1);
        dockWidgetContents->setMinimumSize(QSize(50, 0));
        verticalLayout = new QVBoxLayout(dockWidgetContents);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        frameTreeWidget = new QTreeWidget(dockWidgetContents);
        QTreeWidgetItem *__qtreewidgetitem = new QTreeWidgetItem();
        __qtreewidgetitem->setText(0, QStringLiteral("1"));
        frameTreeWidget->setHeaderItem(__qtreewidgetitem);
        frameTreeWidget->setObjectName(QStringLiteral("frameTreeWidget"));
        frameTreeWidget->setEnabled(true);
        QSizePolicy sizePolicy2(QSizePolicy::Minimum, QSizePolicy::Expanding);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(2);
        sizePolicy2.setHeightForWidth(frameTreeWidget->sizePolicy().hasHeightForWidth());
        frameTreeWidget->setSizePolicy(sizePolicy2);
        frameTreeWidget->setMaximumSize(QSize(16777215, 16777215));
        frameTreeWidget->setSizeIncrement(QSize(0, 1));
        frameTreeWidget->setAcceptDrops(true);
        frameTreeWidget->setIndentation(10);

        verticalLayout->addWidget(frameTreeWidget);

        rasterLayout = new QVBoxLayout();
        rasterLayout->setObjectName(QStringLiteral("rasterLayout"));
        rasterTreeWidget = new QTreeWidget(dockWidgetContents);
        QTreeWidgetItem *__qtreewidgetitem1 = new QTreeWidgetItem();
        __qtreewidgetitem1->setText(0, QStringLiteral("1"));
        rasterTreeWidget->setHeaderItem(__qtreewidgetitem1);
        rasterTreeWidget->setObjectName(QStringLiteral("rasterTreeWidget"));
        QSizePolicy sizePolicy3(QSizePolicy::Minimum, QSizePolicy::Ignored);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(1);
        sizePolicy3.setHeightForWidth(rasterTreeWidget->sizePolicy().hasHeightForWidth());
        rasterTreeWidget->setSizePolicy(sizePolicy3);
        rasterTreeWidget->setIndentation(10);

        rasterLayout->addWidget(rasterTreeWidget);


        verticalLayout->addLayout(rasterLayout);

        subLayout = new QVBoxLayout();
        subLayout->setObjectName(QStringLiteral("subLayout"));
        decParsTree = new QTreeWidget(dockWidgetContents);
        QTreeWidgetItem *__qtreewidgetitem2 = new QTreeWidgetItem();
        __qtreewidgetitem2->setText(0, QStringLiteral("1"));
        decParsTree->setHeaderItem(__qtreewidgetitem2);
        decParsTree->setObjectName(QStringLiteral("decParsTree"));
        QSizePolicy sizePolicy4(QSizePolicy::Minimum, QSizePolicy::Ignored);
        sizePolicy4.setHorizontalStretch(0);
        sizePolicy4.setVerticalStretch(0);
        sizePolicy4.setHeightForWidth(decParsTree->sizePolicy().hasHeightForWidth());
        decParsTree->setSizePolicy(sizePolicy4);
        decParsTree->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        decParsTree->setIndentation(10);
        decParsTree->header()->setVisible(false);

        subLayout->addWidget(decParsTree);


        verticalLayout->addLayout(subLayout);

        logPlainTextEdit = new QPlainTextEdit(dockWidgetContents);
        logPlainTextEdit->setObjectName(QStringLiteral("logPlainTextEdit"));
        QSizePolicy sizePolicy5(QSizePolicy::Minimum, QSizePolicy::Expanding);
        sizePolicy5.setHorizontalStretch(0);
        sizePolicy5.setVerticalStretch(1);
        sizePolicy5.setHeightForWidth(logPlainTextEdit->sizePolicy().hasHeightForWidth());
        logPlainTextEdit->setSizePolicy(sizePolicy5);

        verticalLayout->addWidget(logPlainTextEdit);

        layerDialog->setWidget(dockWidgetContents);

        retranslateUi(layerDialog);

        QMetaObject::connectSlotsByName(layerDialog);
    } // setupUi

    void retranslateUi(QDockWidget *layerDialog)
    {
        layerDialog->setWindowTitle(QApplication::translate("layerDialog", "Layer Dialog", 0));
    } // retranslateUi

};

namespace Ui {
    class layerDialog: public Ui_layerDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LAYERDIALOG_H
