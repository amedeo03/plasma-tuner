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
};

QTEST_GUILESS_MAIN(NotesTest)
#include "notestest.moc"
