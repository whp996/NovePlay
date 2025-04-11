#ifndef PLAYER_H
#define PLAYER_H
#include <QTextToSpeech>
#include <QVoice>
#include <QWidget>

class Player : public QWidget
{
    Q_OBJECT
public:
    static Player* getInstance(QWidget *parent = nullptr) {
        static Player instance(parent);
        return &instance;
    }

    void stopNovelReading();
    bool startNovelReading(QByteArray novel);
    QVector<QVoice> getVoices(){if(m_tts) return m_tts->availableVoices();};
    void setVoice(QVoice voice){if(m_tts) m_tts->setVoice(voice);};
    void Setrate(int rate){if(m_tts) m_tts->setRate(rate);};
    void ContinueNovelReading();
signals:
    void novelFinished();
private:
    Player(QWidget *parent = nullptr);  // 私有构造函数
    ~Player();                          // 私有析构函数
    Player(const Player&) = delete;     // 禁止拷贝
    Player& operator=(const Player&) = delete; // 禁止赋值

    QStringList m_novelSegments;
    int m_currentSegmentIndex = 0;
    bool m_stop = false;

    QTextToSpeech *m_tts = nullptr;
    void readNextSegment();
};

#endif // PLAYER_H
