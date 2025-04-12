#include "player.h"

Player::Player(QWidget *parent) : QWidget(parent)
{
    m_tts = new QTextToSpeech(this);
    m_tts->setLocale(QLocale::Chinese);
    connect(m_tts, &QTextToSpeech::stateChanged, this, [=](QTextToSpeech::State state){
        if(state == QTextToSpeech::Ready) readNextSegment();
    });
}

Player::~Player()
{
    delete m_tts;
}


bool Player::startNovelReading(QByteArray novel)
{
    QString novelContent = QString::fromUtf8(novel);

    if(m_tts->state() != QTextToSpeech::Ready) {
        stopNovelReading();
    }
    m_novelSegments.clear();
    m_stop = false;
    // 按段落分割小说内容，这里使用两个换行符作为分割标记
    m_novelSegments = novelContent.split("\n\n", Qt::SkipEmptyParts);
    m_currentSegmentIndex = 0;

    // 开始朗读第一段（如果存在段落）
    if (!m_novelSegments.isEmpty()) {
        readNextSegment();
    } else {
        qDebug() << "小说文件没有内容";
        return false;
    }
    return true;
}

void Player::readNextSegment()
{
    if (m_stop) return;
    if (m_currentSegmentIndex < m_novelSegments.size()) {
        qDebug() << "朗读下一段:" << m_novelSegments.at(m_currentSegmentIndex);
        m_tts->say(m_novelSegments.at(m_currentSegmentIndex));
        m_currentSegmentIndex++;
    } else {
        qDebug() << "全部段落朗读完毕";
        emit novelFinished();
    }
}

void Player::stopNovelReading()
{
    m_stop = true;
    m_tts->stop();
}

void Player::ContinueNovelReading()
{
    if(m_tts) m_stop = false;
    if(m_currentSegmentIndex > 0) m_currentSegmentIndex--;
    if(m_currentSegmentIndex < m_novelSegments.size()) {
        readNextSegment();
    } else {
        qDebug() << "全部段落朗读完毕";
        emit novelFinished();
    }
}