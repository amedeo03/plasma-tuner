#include "core/notes.h"

#include <QTest>

class NotesTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testMidiFromFrequency_data()
    {
        QTest::addColumn<double>("frequency");
        QTest::addColumn<double>("reference");
        QTest::addColumn<double>("midi");

        QTest::newRow("A4") << 440.0 << 440.0 << 69.0;
        QTest::newRow("A5") << 880.0 << 440.0 << 81.0;
        QTest::newRow("E2") << 82.4069 << 440.0 << 40.0;
        QTest::newRow("B0") << 30.8677 << 440.0 << 23.0;
        QTest::newRow("A4 at 432") << 432.0 << 432.0 << 69.0;
        QTest::newRow("440 Hz at A=432 is sharp") << 440.0 << 432.0 << 69.0 + 12.0 * std::log2(440.0 / 432.0);
    }

    void testMidiFromFrequency()
    {
        QFETCH(double, frequency);
        QFETCH(double, reference);
        QFETCH(double, midi);
        QVERIFY(std::abs(Notes::midiFromFrequency(frequency, reference) - midi) < 1e-3);
        QVERIFY(std::abs(Notes::frequencyFromMidi(midi, reference) - frequency) < 1e-3);
    }

    void testNames_data()
    {
        QTest::addColumn<int>("midi");
        QTest::addColumn<QString>("fullName");

        QTest::newRow("middle C") << 60 << QStringLiteral("C4");
        QTest::newRow("A4") << 69 << QStringLiteral("A4");
        QTest::newRow("B3") << 59 << QStringLiteral("B3");
        QTest::newRow("F#1") << 30 << QStringLiteral("F♯1");
        QTest::newRow("B0") << 23 << QStringLiteral("B0");
        QTest::newRow("G0") << 19 << QStringLiteral("G0");
        QTest::newRow("C-1") << 0 << QStringLiteral("C-1");
    }

    void testNames()
    {
        QFETCH(int, midi);
        QFETCH(QString, fullName);
        QCOMPARE(Notes::fullName(midi), fullName);
    }

    void testParse_data()
    {
        QTest::addColumn<QString>("text");
        QTest::addColumn<int>("midi"); // -1 when invalid
        QTest::addColumn<QString>("label");

        QTest::newRow("E2") << "E2" << 40 << "E2";
        QTest::newRow("sharp") << "F#1" << 30 << "F♯1";
        QTest::newRow("flat") << "Bb0" << 22 << "B♭0";
        QTest::newRow("B#3 is C4") << "B#3" << 60 << "B♯3";
        QTest::newRow("Cb4 is B3") << "Cb4" << 59 << "C♭4";
        QTest::newRow("lowest C0") << "C0" << 12 << "C0";
        QTest::newRow("highest C8") << "C8" << 108 << "C8";
        QTest::newRow("below C0") << "Cb0" << -1 << "";
        QTest::newRow("above C8") << "C#8" << -1 << "";
        QTest::newRow("octave 9") << "E9" << -1 << "";
        QTest::newRow("bad letter") << "Z9" << -1 << "";
        QTest::newRow("H") << "H2" << -1 << "";
        QTest::newRow("no octave") << "E" << -1 << "";
        QTest::newRow("lowercase") << "e2" << -1 << "";
        QTest::newRow("unicode sharp") << "F♯1" << -1 << "";
        QTest::newRow("unicode flat") << "B♭0" << -1 << "";
        QTest::newRow("double sharp") << "F##1" << -1 << "";
        QTest::newRow("negative octave") << "E-1" << -1 << "";
        QTest::newRow("spaces") << " E2" << -1 << "";
        QTest::newRow("empty") << "" << -1 << "";
    }

    void testParse()
    {
        QFETCH(QString, text);
        QFETCH(int, midi);
        QFETCH(QString, label);

        const auto note = Notes::parse(text);
        QCOMPARE(note.has_value(), midi >= 0);
        if (note) {
            QCOMPARE(note->midi, midi);
            QCOMPARE(note->label, label);
        }
    }
};

QTEST_GUILESS_MAIN(NotesTest)
#include "notestest.moc"
