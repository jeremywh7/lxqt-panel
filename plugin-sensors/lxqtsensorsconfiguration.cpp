/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXDE-Qt - a lightweight, Qt based, desktop toolset
 * http://razor-qt.org
 *
 * Copyright: 2012 Razor team
 * Authors:
 *   Łukasz Twarduś <ltwardus@gmail.com>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include "lxqtsensorsconfiguration.h"
#include "ui_lxqtsensorsconfiguration.h"
#include <QCheckBox>
#include <QColorDialog>
#include <QDebug>
#include <QPushButton>
#include <QStringList>
#include "../panel/pluginsettings.h"


namespace
{
    QStringList getChildGroups(PluginSettings const & settings, QString parent)
    {
        static const QRegExp reg_subgroup{"^([^/]*)/.+$"};
        QStringList groups;
        QStringList all = settings.allKeys();
        parent.push_back(QLatin1Char('/'));
        for (auto const & key : all)
        {
            if (0 == key.indexOf(parent))
            {
                QString subkey = key.mid(parent.size());
                subkey.replace(reg_subgroup, QStringLiteral("\\1"));
                if (groups.cend() == std::find(groups.cbegin(), groups.cend(), subkey))
                    groups.push_back(subkey);
            }
        }
        return groups;
    }
}

LXQtSensorsConfiguration::LXQtSensorsConfiguration(PluginSettings * settings, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LXQtSensorsConfiguration),
    mSettings(settings)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setObjectName("SensorsConfigurationWindow");
    ui->setupUi(this);

    // We load settings here cause we have to set up dynamic widgets
    loadSettings();

    connect(ui->buttons, SIGNAL(clicked(QAbstractButton*)), this, SLOT(dialogButtonsAction(QAbstractButton*)));
    connect(ui->updateIntervalSB, SIGNAL(valueChanged(int)), this, SLOT(saveSettings()));
    connect(ui->tempBarWidthSB, SIGNAL(valueChanged(int)), this, SLOT(saveSettings()));
    connect(ui->detectedChipsCB, SIGNAL(activated(int)), this, SLOT(detectedChipSelected(int)));
    connect(ui->celsiusTempScaleRB, SIGNAL(toggled(bool)), this, SLOT(saveSettings()));
    // We don't need signal from the other radio box as celsiusTempScaleRB will send one
    //connect(ui->fahrenheitTempScaleRB, SIGNAL(toggled(bool)), this, SLOT(saveSettings()));
    connect(ui->warningAboutHighTemperatureChB, SIGNAL(toggled(bool)), this, SLOT(saveSettings()));

    /**
     * Signals for enable/disable and bar color change are set in the loadSettings method because
     * we are creating them dynamically.
     */
}


LXQtSensorsConfiguration::~LXQtSensorsConfiguration()
{
    delete ui;
}


void LXQtSensorsConfiguration::loadSettings()
{
    ui->updateIntervalSB->setValue(mSettings->value("updateInterval").toInt());
    ui->tempBarWidthSB->setValue(mSettings->value("tempBarWidth").toInt());

    if (mSettings->value("useFahrenheitScale").toBool())
    {
        ui->fahrenheitTempScaleRB->setChecked(true);
    }

    // In case of reloading settings we have to clear GUI elements
    ui->detectedChipsCB->clear();

    QStringList chipNames = getChildGroups(*mSettings, QStringLiteral("chips"));

    for (int i = 0; i < chipNames.size(); ++i)
    {
        ui->detectedChipsCB->addItem(chipNames[i]);
    }

    // Load feature for the first chip if exist
    if (chipNames.size() > 0)
    {
        detectedChipSelected(0);
    }

    ui->warningAboutHighTemperatureChB->setChecked(
            mSettings->value("warningAboutHighTemperature").toBool());
}


void LXQtSensorsConfiguration::saveSettings()
{
    mSettings->setValue("updateInterval", ui->updateIntervalSB->value());
    mSettings->setValue("tempBarWidth", ui->tempBarWidthSB->value());

    if (ui->fahrenheitTempScaleRB->isChecked())
    {
        mSettings->setValue("useFahrenheitScale", true);
    }
    else
    {
        mSettings->setValue("useFahrenheitScale", false);
    }

    QStringList group = { QStringList("chips") };
    QStringList chipNames = getChildGroups(*mSettings, group.join(QStringLiteral("/")));

    if (chipNames.size())
    {
        QStringList chipFeatureLabels;
        QPushButton* colorButton = NULL;
        QCheckBox* enabledCheckbox = NULL;

        group.push_back(chipNames[ui->detectedChipsCB->currentIndex()]);

        chipFeatureLabels = getChildGroups(*mSettings, group.join(QStringLiteral("/")));
        for (int j = 0; j < chipFeatureLabels.size(); ++j)
        {
            group.push_back(chipFeatureLabels[j]);

            enabledCheckbox = qobject_cast<QCheckBox*>(ui->chipFeaturesT->cellWidget(j, 0));
            // We know what we are doing so we don't have to check if enabledCheckbox == 0
            group.push_back(QStringLiteral("enabled"));
            mSettings->setValue(group.join(QStringLiteral("/")), enabledCheckbox->isChecked());
            group.pop_back();

            colorButton = qobject_cast<QPushButton*>(ui->chipFeaturesT->cellWidget(j, 2));
            // We know what we are doing so we don't have to check if colorButton == 0
            group.push_back(QStringLiteral("color"));
            mSettings->setValue(group.join(QStringLiteral("/")),
                    colorButton->palette().color(QPalette::Normal, QPalette::Button).name());
            group.pop_back();

            group.pop_back();
        }
        group.pop_back();

    }

    mSettings->setValue("warningAboutHighTemperature",
                       ui->warningAboutHighTemperatureChB->isChecked());
}


void LXQtSensorsConfiguration::dialogButtonsAction(QAbstractButton *btn)
{
    if (ui->buttons->buttonRole(btn) == QDialogButtonBox::ResetRole)
    {
        mSettings->loadFromCache();
        loadSettings();
    }
    else
    {
        close();
    }
}


void LXQtSensorsConfiguration::changeProgressBarColor()
{
    QAbstractButton* btn = qobject_cast<QAbstractButton*>(sender());

    if (btn)
    {
        QPalette pal = btn->palette();
        QColor color = QColorDialog::getColor(pal.color(QPalette::Normal, QPalette::Button), this);

        if (color.isValid())
        {
            pal.setColor(QPalette::Normal, QPalette::Button, color);
            btn->setPalette(pal);
            saveSettings();
        }
    }
    else
    {
        qDebug() << "LXQtSensorsConfiguration::changeProgressBarColor():" << "invalid button cast";
    }
}


void LXQtSensorsConfiguration::detectedChipSelected(int index)
{
    QStringList group = { QStringLiteral("chips") };
    QStringList chipNames = getChildGroups(*mSettings, group.join(QStringLiteral("/")));
    QStringList chipFeatureLabels;
    QPushButton* colorButton = NULL;
    QCheckBox* enabledCheckbox = NULL;
    QTableWidgetItem *chipFeatureLabel = NULL;

    if (index < chipNames.size())
    {
        qDebug() << "Selected chip: " << ui->detectedChipsCB->currentText();

        // In case of reloading settings we have to clear GUI elements
        ui->chipFeaturesT->setRowCount(0);

        // Add detected chips and features
        QStringList chipFeaturesLabels;
        chipFeaturesLabels << tr("Enabled") << tr("Label") << tr("Color");
        ui->chipFeaturesT->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        ui->chipFeaturesT->setHorizontalHeaderLabels(chipFeaturesLabels);

        group.push_back(chipNames[index]);
        chipFeatureLabels = getChildGroups(*mSettings, group.join(QStringLiteral("/")));
        for (int j = 0; j < chipFeatureLabels.size(); ++j)
        {
            group.push_back(chipFeatureLabels[j]);

            ui->chipFeaturesT->insertRow(j);

            enabledCheckbox = new QCheckBox(ui->chipFeaturesT);
            group.push_back(QStringLiteral("enabled"));
            enabledCheckbox->setChecked(mSettings->value(group.join(QStringLiteral("/"))).toBool());
            group.pop_back();
            // Connect here after the setChecked call because we don't want to send signal
            connect(enabledCheckbox, SIGNAL(stateChanged(int)), this, SLOT(saveSettings()));
            ui->chipFeaturesT->setCellWidget(j, 0, enabledCheckbox);

            chipFeatureLabel = new QTableWidgetItem(chipFeatureLabels[j]);
            chipFeatureLabel->setFlags(Qt::ItemIsEnabled);
            ui->chipFeaturesT->setItem(j, 1, chipFeatureLabel);

            colorButton = new QPushButton(ui->chipFeaturesT);
            connect(colorButton, SIGNAL(clicked()), this, SLOT(changeProgressBarColor()));
            QPalette pal = colorButton->palette();
            group.push_back(QStringLiteral("color"));
            pal.setColor(QPalette::Normal, QPalette::Button,
                         QColor(mSettings->value(group.join(QStringLiteral("/"))).toString()));
            group.pop_back();
            colorButton->setPalette(pal);
            ui->chipFeaturesT->setCellWidget(j, 2, colorButton);

            group.pop_back();
        }
        group.pop_back();
    }
    else
    {
        qDebug() << "Invalid chip index: " << index;
    }

}
