/*
 *
 *   Copyright (C) 2009 Ian Reinhart Geiser <geiseri@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License versin 2 as
 *   published by the Free Software Foundation.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.
 */

 #define KCONFIGRAWEDITORTEST
 #include "kconfigraweditor.cpp"

 #include <qdebug.h>

 int main(int argc, char **argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    KConfigRawEditor conf("testconfig");

    if( conf.load() )
    {
        qDebug() << "Is mutable" << conf.group("General")->isMutable;
        qDebug() << "Normal value" << conf.group("General")->data["widgetStyle"].value
            << conf.group("General")->data["widgetStyle"].type;
	qDebug() << "Immutable value" << conf.group("NormalGroup")->data["ImmutableValue"].value
	    << conf.group("NormalGroup")->data["ImmutableValue"].type;
	qDebug() << "ShellExpanded value" << conf.group("NormalGroup")->data["ShellExpandedValue"].value
	    << conf.group("NormalGroup")->data["ShellExpandedValue"].type;
	qDebug() << "ShellExpandedAndImmutable value" << conf.group("NormalGroup")->data["ShellExpandedAndImmutableValue"].value
	    << conf.group("NormalGroup")->data["ShellExpandedAndImmutableValue"].type;

	qDebug() << "Is not mutable" << conf.group("ImutableGroup")->isMutable;
		qDebug() << "Normal value" << conf.group("ImutableGroup")->data["NormalValue"].value
	    << conf.group("ImutableGroup")->data["NormalValue"].type;
	qDebug() << "ShellExpanded value" << conf.group("ImutableGroup")->data["ShellExpandedValue"].value
	    << conf.group("ImutableGroup")->data["ShellExpandedValue"].type;
    }

     KConfigRawEditor conf2("testconfig-out");
     conf2.group("General")->isMutable = true;
     conf2.group("General")->data["Normal Value"] = KConfigRawEditor::KConfigEntryData( "Normal Value", KConfigRawEditor::KConfigEntryData::Normal );
     conf2.group("General")->data["Nother Value"] = KConfigRawEditor::KConfigEntryData( "Nother Normal Value", KConfigRawEditor::KConfigEntryData::Normal );

     conf2.save();

    return 0;
}
