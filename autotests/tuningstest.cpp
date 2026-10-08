#include "core/notes.h"
#include "core/tunings.h"

#include <QTest>

using Tunings::Instrument;

namespace
{
QString notesString(Instrument instrument, int strings, int tuning)
{
    QStringList names;
    for (const int midi : Tunings::tuningNotes(instrument, strings, tuning)) {
        names.append(Notes::fullName(midi));
    }
    return names.join(QLatin1Char(' '));
}
}

class TuningsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testStringCounts()
    {
        QCOMPARE(Tunings::stringCounts(Instrument::Guitar), QList<int>({6, 7, 8}));
        QCOMPARE(Tunings::stringCounts(Instrument::Bass), QList<int>({4, 5, 6}));
    }

    void testNotes_data()
    {
        QTest::addColumn<Instrument>("instrument");
        QTest::addColumn<int>("strings");
        QTest::addColumn<int>("tuning");
        QTest::addColumn<QString>("name");
        QTest::addColumn<QString>("notes");

        QTest::newRow("guitar E standard") << Instrument::Guitar << 6 << 0 << "E standard" << "E2 A2 D3 G3 B3 E4";
        QTest::newRow("guitar drop D") << Instrument::Guitar << 6 << 1 << "Drop D" << "D2 A2 D3 G3 B3 E4";
        QTest::newRow("guitar D standard") << Instrument::Guitar << 6 << 2 << "D standard" << "D2 G2 C3 F3 A3 D4";
        QTest::newRow("guitar drop C") << Instrument::Guitar << 6 << 3 << "Drop C" << "C2 G2 C3 F3 A3 D4";
        QTest::newRow("guitar C standard") << Instrument::Guitar << 6 << 4 << "C standard" << "C2 F2 A♯2 D♯3 G3 C4";
        QTest::newRow("7-string standard") << Instrument::Guitar << 7 << 0 << "E standard" << "B1 E2 A2 D3 G3 B3 E4";
        QTest::newRow("7-string drop A") << Instrument::Guitar << 7 << 1 << "Drop A" << "A1 E2 A2 D3 G3 B3 E4";
        QTest::newRow("8-string standard") << Instrument::Guitar << 8 << 0 << "E standard" << "F♯1 B1 E2 A2 D3 G3 B3 E4";
        QTest::newRow("8-string drop E") << Instrument::Guitar << 8 << 1 << "Drop E" << "E1 B1 E2 A2 D3 G3 B3 E4";
        QTest::newRow("bass E standard") << Instrument::Bass << 4 << 0 << "E standard" << "E1 A1 D2 G2";
        QTest::newRow("bass drop D") << Instrument::Bass << 4 << 1 << "Drop D" << "D1 A1 D2 G2";
        QTest::newRow("bass C standard") << Instrument::Bass << 4 << 4 << "C standard" << "C1 F1 A♯1 D♯2";
        QTest::newRow("5-string bass") << Instrument::Bass << 5 << 0 << "E standard" << "B0 E1 A1 D2 G2";
        QTest::newRow("6-string bass") << Instrument::Bass << 6 << 0 << "E standard" << "B0 E1 A1 D2 G2 C3";
        QTest::newRow("6-string bass drop") << Instrument::Bass << 6 << 3 << "Drop G" << "G0 D1 G1 C2 F2 A♯2";
    }

    void testNotes()
    {
        QFETCH(Instrument, instrument);
        QFETCH(int, strings);
        QFETCH(int, tuning);
        QFETCH(QString, name);
        QFETCH(QString, notes);
        QCOMPARE(Tunings::tuningName(instrument, strings, tuning), name);
        QCOMPARE(notesString(instrument, strings, tuning), notes);
    }

    void testInvalid()
    {
        QVERIFY(Tunings::tuningNotes(Instrument::Guitar, 4, 0).isEmpty());
        QVERIFY(Tunings::tuningNotes(Instrument::Bass, 8, 0).isEmpty());
        QVERIFY(Tunings::tuningNotes(Instrument::Guitar, 6, Tunings::tuningCount()).isEmpty());
        QVERIFY(Tunings::tuningName(Instrument::Guitar, 6, -1).isEmpty());
    }
};

QTEST_GUILESS_MAIN(TuningsTest)
#include "tuningstest.moc"
