#include "Button_pullup.h"

Button::Button(int p, bool c = false)
{
    pin = p;
    config = c;
    if (config == true) pinMode(pin, INPUT_PULLUP);
    else pinMode(pin, INPUT_PULLDOWN);
}

void Button::SetFlank(bool f)
{
    flank = f;
    previousState = !flank;
}

bool Button::GetIsPress()
{
    bool actualState = digitalRead(pin);
    bool state = (previousState != actualState) && actualState == flank;
    previousState = actualState;
    delay(100);
    return state;
}
