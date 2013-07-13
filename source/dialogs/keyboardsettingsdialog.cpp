/*
  * Copyright (C) 2011 Cameron White
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
  
#include "keyboardsettingsdialog.h"
#include "ui_keyboardsettingsdialog.h"

#include <app/command.h>
#include <QKeyEvent>

Q_DECLARE_METATYPE(Command *)

KeyboardSettingsDialog::KeyboardSettingsDialog(QWidget *parent,
                                               const QList<Command *> &commands)
    : QDialog(parent),
      ui(new Ui::KeyboardSettingsDialog),
      myCommands(commands)
{
    ui->setupUi(this);

    initializeCommandTable();

    ui->shortcutEdit->installEventFilter(this);

    connect(ui->resetButton, SIGNAL(clicked()), this, SLOT(resetShortcut()));

    connect(ui->commandsList,
            SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
            this,
            SLOT(activeCommandChanged(QTreeWidgetItem *,QTreeWidgetItem *)));

    connect(ui->defaultButton, SIGNAL(clicked()), this,
            SLOT(resetToDefaultShortcut()));
}

KeyboardSettingsDialog::~KeyboardSettingsDialog()
{
    delete ui;
}

namespace
{
bool compareCommands(const Command *cmd1, const Command *cmd2)
{
    return cmd1->id() < cmd2->id();
}
}

void KeyboardSettingsDialog::initializeCommandTable()
{
    qSort(myCommands.begin(), myCommands.end(), compareCommands);

    ui->commandsList->setColumnCount(3);

    ui->commandsList->setHeaderLabels(QStringList() << tr("Command")
                                      << tr("Label") << tr("Shortcut"));

    // Populate list of commands.
    foreach(Command *command, myCommands)
    {
        // NOTE: QAction::toolTip() is called to avoid getting ampersands from
        //       mnemonics (which would appear in QAction::text)
        QTreeWidgetItem* item = new QTreeWidgetItem(
                    QStringList() << command->id() << command->toolTip()
                    << command->shortcut().toString());

        item->setData(0, Qt::UserRole, qVariantFromValue(command));
        ui->commandsList->addTopLevelItem(item);
    }

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    ui->commandsList->header()->sectionResizeMode(QHeaderView::ResizeToContents);
#else
    ui->commandsList->header()->setResizeMode(QHeaderView::ResizeToContents);
#endif

    // resize dialog to avoid horizontal scrollbars
    int totalWidth = 0;
    for (int i = 0; i < ui->commandsList->columnCount(); i++)
    {
        totalWidth += ui->commandsList->columnWidth(i);
    }

    resize(totalWidth + 50, height());

    ui->commandsList->setCurrentItem(ui->commandsList->itemAt(0, 0));
}

bool KeyboardSettingsDialog::eventFilter(QObject *, QEvent *e)
{
    if (e->type() == QEvent::KeyPress)
    {
        QKeyEvent *k = static_cast<QKeyEvent *>(e);
        processKeyPress(k);
        return true;
    }

    // Ignore these events.
    if (e->type() == QEvent::KeyRelease)
    {
        return true;
    }

    return false;
}

void KeyboardSettingsDialog::processKeyPress(QKeyEvent *e)
{
    int key = e->key();

    // Ignore a modifer key by itself (i.e. just the Ctrl key).
    if (key == Qt::Key_Control || key == Qt::Key_Shift ||
            key == Qt::Key_Meta || key == Qt::Key_Alt)
    {
        return;
    }

    // Allow the use of backspace to clear the shortcut.
    if (key == Qt::Key_Backspace)
    {
        setShortcut("");
    }
    else
    {
        // Add in any modifers like Shift or Ctrl, but remove the keypad modifer
        // since QKeySequence doesn't handle that well.
        key |= (e->modifiers() & ~Qt::KeypadModifier);

        QKeySequence newKeySequence(key);
        setShortcut(newKeySequence.toString());
    }

    e->accept();
}

void KeyboardSettingsDialog::resetShortcut()
{
    setShortcut(activeCommand()->shortcut().toString());
}

void KeyboardSettingsDialog::resetToDefaultShortcut()
{
    setShortcut(activeCommand()->defaultShortcut().toString());
}

void KeyboardSettingsDialog::setShortcut(const QString& shortcut)
{
    ui->commandsList->currentItem()->setText(CommandShortcut, shortcut);
    ui->shortcutEdit->setText(shortcut);
}

void KeyboardSettingsDialog::activeCommandChanged(QTreeWidgetItem* current,
                                                  QTreeWidgetItem* /*previous*/)
{
    ui->shortcutEdit->setText(current->text(CommandShortcut));
}

void KeyboardSettingsDialog::saveShortcuts()
{
    for (int i = 0; i < ui->commandsList->topLevelItemCount(); i++)
    {
        QTreeWidgetItem *currentItem = ui->commandsList->topLevelItem(i);
        Command* command = currentItem->data(0, Qt::UserRole).value<Command*>();

        command->setShortcut(currentItem->text(CommandShortcut));
    }
}

void KeyboardSettingsDialog::accept()
{
    saveShortcuts();
    done(Accepted);
}

Command *KeyboardSettingsDialog::activeCommand() const
{
    return ui->commandsList->currentItem()->data(0, Qt::UserRole).value<Command*>();
}
