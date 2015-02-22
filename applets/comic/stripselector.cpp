/***************************************************************************
 *   Copyright (C) 2012 Matthias Fuchs <mat69@gmx.net>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "stripselector.h"
#include "stripselector_p.h"
#include "comicdata.h"

#include <KDatePicker>
#include <QDialog>
#include <QDialogButtonBox>
#include <QInputDialog>
#include <KNumInput>
#include <KLocalizedString>

#include <QtCore/QScopedPointer>
#include <QtCore/QTimer>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>

//NOTE based on GotoPageDialog KDE/kdegraphics/okular/part.cpp
//BEGIN choose a strip dialog
class ChooseStripNumDialog : public QDialog
{
    public:
        ChooseStripNumDialog(QWidget *parent, int current, int min, int max)
            : QDialog( parent )
        {
            setWindowTitle(i18n("Go to Strip"));

            QVBoxLayout *topLayout = new QVBoxLayout(this);
            topLayout->setMargin(0);
            numInput = new KIntNumInput(current, this);
            numInput->setRange(min, max);
            numInput->setEditFocus(true);
            numInput->setSliderEnabled(true);

            QLabel *label = new QLabel(i18n("&Strip Number:"), this);
            label->setBuddy(numInput);
            topLayout->addWidget(label);
            topLayout->addWidget(numInput) ;
            // A little bit extra space
            topLayout->addStretch(10);
            
            QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
            buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
            connect(buttonBox, SIGNAL(accepted()), SLOT(accept()));
            connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));
            topLayout->addWidget(buttonBox);
            
            numInput->setFocus();
        }

        int getStripNumber() const
        {
            return numInput->value();
        }

    protected:
        KIntNumInput *numInput;
};
//END choose a strip dialog

StripSelector::StripSelector(QObject *parent)
  : QObject(parent)
{
}

StripSelector::~StripSelector()
{
}

StripSelector *StripSelectorFactory::create(IdentifierType type)
{
    switch (type) {
        case Number:
            return new NumberStripSelector();
        case Date:
            return new DateStripSelector();
        case String:
            return new StringStripSelector();
    }

    return 0;
}


StringStripSelector::StringStripSelector(QObject *parent)
  : StripSelector(parent)
{
}

StringStripSelector::~StringStripSelector()
{
}

void StringStripSelector::select(const ComicData &currentStrip)
{
    bool ok;
    const QString strip = QInputDialog::getText(0, i18n("Go to Strip"), i18n("Strip identifier:"), QLineEdit::Normal,
                                                 currentStrip.current(), &ok);
    if (ok) {
        emit stripChosen(strip);
    }
    deleteLater();
}

NumberStripSelector::NumberStripSelector(QObject *parent)
  : StripSelector(parent)
{
}

NumberStripSelector::~NumberStripSelector()
{
}

void NumberStripSelector::select(const ComicData &currentStrip)
{
    QScopedPointer<ChooseStripNumDialog> pageDialog(new ChooseStripNumDialog(0, currentStrip.current().toInt(),
                                                    currentStrip.firstStripNum(), currentStrip.maxStripNum()));
    if (pageDialog->exec() == QDialog::Accepted) {
        emit stripChosen(QString::number(pageDialog->getStripNumber()));
    }
    deleteLater();
}

DateStripSelector::DateStripSelector(QObject *parent)
  : StripSelector(parent)
{
}

DateStripSelector::~DateStripSelector()
{
}

void DateStripSelector::select(const ComicData &currentStrip)
{
    mFirstIdentifierSuffix = currentStrip.first();

    KDatePicker *calendar = new KDatePicker;
    calendar->setAttribute(Qt::WA_DeleteOnClose);//to have destroyed emitted upon closing
    calendar->setMinimumSize(calendar->sizeHint());
    calendar->setDate(QDate::fromString(currentStrip.current(), "yyyy-MM-dd"));

    connect(calendar, SIGNAL(dateSelected(QDate)), this, SLOT(slotChosenDay(QDate)));
    connect(calendar, SIGNAL(dateEntered(QDate)), this, SLOT(slotChosenDay(QDate)));

    // only delete this if the dialog got closed
    connect(calendar, SIGNAL(destroyed(QObject*)), this, SLOT(deleteLater()));
    calendar->show();
}

void DateStripSelector::slotChosenDay(const QDate &date)
{
    if (date <= QDate::currentDate()) {
        QDate temp = QDate::fromString(mFirstIdentifierSuffix, "yyyy-MM-dd");
        // only update if date >= first strip date, or if there is no first
        // strip date
        if (temp.isValid() || date >= temp) {
            emit stripChosen(date.toString("yyyy-MM-dd"));
        }
    }
}

#include "moc_stripselector.cpp"
#include "moc_stripselector_p.cpp"
