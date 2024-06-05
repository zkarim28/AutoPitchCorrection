/*
     _______         __  __  __             _____    __       __
    |       |.--.--.|__||  ||__|.-----.    |     |_ |  |_ .--|  |    Copyright 2023
    |   -  _||  |  ||  ||  ||  ||  _  |    |       ||   _||  _  |    Quilio Limited
    |_______||_____||__||__||__||_____|    |_______||____||_____|    www.quilio.dev
                                                             
    Quilio Software uses a commercial licence     -     see LICENCE.md for details.
*/

#ifndef Scales_h
#define Scales_h

class Scales{
    
public:
    enum ScaleNames{
        Chromatic=0,
        Major,
        Minor,
        Dorian,
        Mixolydian,
        Lydian,
        Phrygian,
        Locrian,
        HarmonicMinor,
        MelodicMinor,
        MajorPentatonic,
        MinorPentatonic,
        MinorBlues
    };
    
    enum Notes{
        NoteC=0,
        NoteDb,
        NoteD,
        NoteEb,
        NoteE,
        NoteF,
        NoteGb,
        NoteG,
        NoteAb,
        NoteA,
        NoteBb,
        NoteB
    };
    
    Scales(){};
    
    ~Scales(){};
    
    void makeScale(int root, int scaleType, int arr[]){
        int dst = root;
        for (int src=0; src<12; src++, dst++) {
            if (dst >= 12) {
                dst -= 12;
            }
            arr[dst] = scaleNotes[scaleType][src];
        }
    }

private:
    const int scaleNotes[13][12] = {
        {1,1,1,1,1,1,1,1,1,1,1,1}, //Chromatic
        {1,0,1,0,1,1,0,1,0,1,0,1}, //Major
        {1,0,1,1,0,1,0,1,1,0,1,0}, //Minor
        {1,0,1,1,0,1,0,1,0,1,1,0}, //Dorian
        {1,0,1,0,1,1,0,1,0,1,1,0}, //Mixolydian
        {1,0,1,0,1,0,1,1,0,1,0,1}, //Lydian
        {1,1,0,1,0,1,0,1,1,0,1,0}, //Phyrgian
        {1,1,0,1,0,1,1,0,1,0,1,0}, //Locrian
        {1,0,1,1,0,1,0,1,1,0,0,1}, //Harmonic Minor
        {1,0,1,1,0,1,0,1,0,1,0,1}, //Melodic Minor
        {1,0,1,0,1,0,0,1,0,1,0,0}, //Maj Pentatonic
        {1,0,0,1,0,1,0,1,0,0,1,0}, //Min Pentatonic
        {1,0,0,1,0,1,1,1,0,0,1,0}, //Minor Blues
    };
};

#endif /* Scales_h */

