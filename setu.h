#ifndef SETU_H
#define SETU_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class Setu;
}
QT_END_NAMESPACE

class Setu : public QWidget
{
    Q_OBJECT

public:
    Setu(QWidget *parent = nullptr);
    ~Setu();

private slots:
    void handleButtonClicked();  // Slot for button click
    void handleClearButtonClicked();
    void orgPicClicked();

private:
    Ui::Setu *ui;
};
#endif // SETU_H
