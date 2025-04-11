#include "links.h"

Links::Links(QObject *parent)
    : QObject(parent)
{
}

Links::~Links()
{
}

void Links::SetFrontLink(const Link &link)
{
    FrontLink = link;
}

Link Links::GetCurrentLink() const
{
    return CurrentLink;
}

void Links::SetCurrentLink(const Link &link)
{
    CurrentLink = link;
}
