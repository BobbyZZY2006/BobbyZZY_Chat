#include "ChatDialog.h"

ChatDialog::ChatDialog(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::ChatDialogClass())
{
	ui->setupUi(this);
	ui->add_btn->setCursor(Qt::PointingHandCursor);

}

ChatDialog::~ChatDialog()
{
	delete ui;
}
