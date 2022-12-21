//
//  LookAndFeel_frqz_rm.h
//  SeamLessPluginSuite
//
//  Created by Fares Schulz on 06.11.22.
//

#ifndef LookAndFeel_frqz_rm_h
#define LookAndFeel_frqz_rm_h

class LaF   :   public juce::LookAndFeel_V4
{
public:
    juce::Typeface::Ptr Antaro;
    const juce::Colour ColourTextboxBackground = juce::Colour(0x340c44ff);
    const juce::Colour ClBackground = juce::Colour(0xFF2D2D2D);
    const juce::Colour ClFace = juce::Colour(0xFFD8D8D8);
    const juce::Colour ClFaceShadow = juce::Colour(0XFF272727);
    const juce::Colour ClFaceShadowOutline = juce::Colour(0xFF212121);
    const juce::Colour ClFaceShadowOutlineActive = juce::Colour(0xFF7C7C7C);
    const juce::Colour ClRotSliderArrow = juce::Colour(0xFF4A4A4A);
    const juce::Colour ClRotSliderArrowShadow = juce::Colour(0x445D5D5D);
    const juce::Colour ClSliderFace = juce::Colour(0xFF191919);
    const juce::Colour ClText = juce::Colour(0xFFFFFFFF);
    const juce::Colour ClSeperator = juce::Colour(0xFF979797);
    const juce::Colour ClWidgetColours[4] = {
        juce::Colour(0xFF00CAFF), juce::Colour(0xFF4FFF00), juce::Colour(0xFFFF9F00), juce::Colour(0xFFD0011B)};
    
    LaF()
    {
        Antaro = juce::Typeface::createSystemTypefaceFor(BinaryData::Antaro_ttf, BinaryData::Antaro_ttfSize); //TODO: free this data
        setColour(juce::ResizableWindow::backgroundColourId,(juce::Colour(0xb8a6e2ff)));
    }
    
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos, const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider) override
    {
        auto radius = (float) juce::jmin (width / 2, height / 2) - 10.0f;
        auto centreX = (float) x + (float) width  * 0.5f;
        auto centreY = (float) y + (float) height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        
        // fill
        g.setColour (juce::Colours::purple);
        g.fillEllipse(rx, ry, rw, rw);
        
        // outline
        g.setColour(juce::Colours::pink);
        g.drawEllipse(rx, ry, rw, rw, 1.0f);
        
        juce::Path p;
        auto pointerLenght = radius * 0.2f;
        auto pointerThickness = 2.0f;
        p.addRectangle(-pointerThickness*0.5f, -radius, pointerThickness, pointerLenght);
        p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
        
        // pointer
        g.setColour(juce::Colours::lightgrey);
        g.fillPath(p);
        
        slider.setRange(0., 1.);
//        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 20);
    }
    
    juce::Font getLabelFont (juce::Label& label) override
    {
        ignoreUnused (label);
        return juce::Font(Antaro);
    }
    
    void drawLabel (juce::Graphics& g, juce::Label& label) override
    {
        float alpha = label.isEnabled() ? 1.0f : 0.4f;
        g.fillAll (label.findColour (juce::Label::backgroundColourId));
        juce::Rectangle<int> bounds = label.getLocalBounds();
        float edge = 1.0f;
        float x = (float) bounds.getX()+edge;
        float y = (float) bounds.getY();
        float w = (float) bounds.getWidth()-2*edge;
        float h = (float) bounds.getHeight();
        juce::Path p;
        p.addRoundedRectangle(x, y, w, h, h/8.0f);
        g.setColour (ColourTextboxBackground.withMultipliedAlpha(alpha));
        g.fillPath (p);

        if (! label.isBeingEdited())
        {
            const float fontAlpha = label.isEnabled() ? 1.0f : 0.5f;
            const juce::Font font (Antaro);

            g.setColour (ClText.withMultipliedAlpha (fontAlpha));
            g.setFont (Antaro);
            g.setFont (13.f);

            juce::Rectangle<int> textArea (label.getBorderSize().subtractedFrom (label.getLocalBounds()));

            g.drawFittedText (label.getText(), textArea, label.getJustificationType(),
                              juce::jmax (1, (int) (textArea.getHeight() / font.getHeight())),
                              label.getMinimumHorizontalScale());

            g.setColour (label.findColour (juce::Label::outlineColourId).withMultipliedAlpha (fontAlpha));
        }
        else if (label.isEnabled())
        {
            g.setColour (label.findColour (juce::Label::outlineColourId));
        }
    }
//    void setColour(juce::ResizableWindow::backgroundColourId,0x340c44ff) over
};

#endif /* LookAndFeel_frqz_rm_h */
