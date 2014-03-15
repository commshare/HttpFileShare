

#include <QtGui>

#include "window.h"

//! [0]
Window::Window()
{

    createActions();
    createTrayIcon();
	showMessage();
    //connect(showMessageButton, SIGNAL(clicked()), this, SLOT(showMessage()));
    
    connect(trayIcon, SIGNAL(messageClicked()), this, SLOT(messageClicked()));
    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));

	label= new QLabel;
	label->setText("Http Server is running!");

	QFont font;
	font.setFamily(QString::fromUtf8("\345\256\213\344\275\223"));
	font.setPointSize(16);
	font.setBold(false);
	font.setWeight(50);
	label->setFont(font);

	QPushButton * btn = new QPushButton;
	btn->setText(tr("关闭服务器 退出"));
	connect(btn, SIGNAL(clicked()), qApp, SLOT(quit()));

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(label);
	mainLayout->addWidget(btn);


    setLayout(mainLayout);

    trayIcon->show();
    setWindowTitle(tr("Http Server"));
    resize(400, 300);
	
}
//! [0]

//! [1]
void Window::setVisible(bool visible)
{
    minimizeAction->setEnabled(visible);
    //maximizeAction->setEnabled(!isMaximized());
    restoreAction->setEnabled(isMaximized() || !visible);
    QDialog::setVisible(visible);
}
//! [1]

//! [2]
void Window::closeEvent(QCloseEvent *event)
{
    if (trayIcon->isVisible()) {
		showMessage();
       /* QMessageBox::information(this, tr("Systray"),
                                 tr("The program will keep running in the "
                                    "system tray. To terminate the program, "
                                    "choose <b>Quit</b> in the context menu "
                                    "of the system tray entry."));*/
        hide();
        event->ignore();
    }
}

void Window::setIcon(int index)
{
    QIcon icon = iconComboBox->itemIcon(index);
    trayIcon->setIcon(icon);
    setWindowIcon(icon);

    trayIcon->setToolTip(iconComboBox->itemText(index));
}


void Window::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger: break;
    case QSystemTrayIcon::DoubleClick:
        iconClicked();
        break;
    case QSystemTrayIcon::MiddleClick:
        showMessage();
        break;
    default:
        ;
    }
}
//! [4]

//! [5]
void Window::showMessage()
{
	trayIcon->showMessage( tr("Http 服务器正在运行!"),
							tr("右键点击图标查看更多选项."), 
							QSystemTrayIcon::Information,
							2 * 1000);
}
//! [5]

//! [6]
void Window::messageClicked()
{
    
}
//! [6]

void Window::iconClicked()
{
   if (this->isVisible())
   {
	   this->hide();
   }
   else this->show();
	this->activateWindow();
}


void Window::createActions()
{
    minimizeAction = new QAction(tr("Mi&nimize Window"), this);
    connect(minimizeAction, SIGNAL(triggered()), this, SLOT(hide()));

    //maximizeAction = new QAction(tr("Ma&ximize"), this);
    //connect(maximizeAction, SIGNAL(triggered()), this, SLOT(showMaximized()));

    restoreAction = new QAction(tr("&Restore Window"), this);
    connect(restoreAction, SIGNAL(triggered()), this, SLOT(showNormal()));

    quitAction = new QAction(tr("&Quit Server"), this);
    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
}

void Window::createTrayIcon()
{
    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(minimizeAction);
    //trayIconMenu->addAction(maximizeAction);
    trayIconMenu->addAction(restoreAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);

	QIcon icon(":images/server.png");// = iconComboBox->itemIcon(index);
	trayIcon->setIcon(icon);
	setWindowIcon(icon);

	trayIcon->setToolTip("Click here.");
}
