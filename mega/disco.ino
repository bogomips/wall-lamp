void disco(int delayms) {

    uint8_t red = 0;
	uint8_t green = 0;
	uint8_t blue = 0;

    //whole_strip(0x000000);
    for(double hue = 0; hue <= 360; hue+=30) {

        ColorConverter::HslToRgb(hue/360, 1, 0.5, red, green, blue);

        setRgb(red,green,blue);
        delay(delayms);
        //setRgb(0,0,0);
        //delay(delayms);

    }
}

void strobo(int delayms) {

        setRgb(255,255,255);
        delay(delayms);
        setRgb(0,0,0);
        delay(delayms);
}

