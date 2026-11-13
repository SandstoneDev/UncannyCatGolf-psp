#pragma once

#include "profiles.hpp"

#include <psputility.h>

class ProfileKeyboard {
public:
    bool active() const { return target!=nullptr; }
    int error() const { return lastError; }

    bool begin(ProfileName& name,const char* prompt,bool russian) {
        if (target) return false;
        field={};
        params={};
        encode(name.data(),input);
        encode(prompt,description);
        output={};
        field.language=russian?PSP_UTILITY_OSK_LANGUAGE_RUSSIAN:PSP_UTILITY_OSK_LANGUAGE_ENGLISH;
        field.inputtype=PSP_UTILITY_OSK_INPUTTYPE_LATIN_DIGIT | PSP_UTILITY_OSK_INPUTTYPE_LATIN_SYMBOL |
                        PSP_UTILITY_OSK_INPUTTYPE_LATIN_LOWERCASE | PSP_UTILITY_OSK_INPUTTYPE_LATIN_UPPERCASE |
                        PSP_UTILITY_OSK_INPUTTYPE_RUSSIAN_LOWERCASE | PSP_UTILITY_OSK_INPUTTYPE_RUSSIAN_UPPERCASE;
        field.lines=1;
        field.desc=description.data();
        field.intext=input.data();
        field.outtextlength=output.size();
        field.outtextlimit=ProfileNameLength;
        field.outtext=output.data();
        params.base.size=sizeof params;
        sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE,&params.base.language);
        sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN,&params.base.buttonSwap);
        params.base.graphicsThread=17;
        params.base.accessThread=19;
        params.base.fontThread=18;
        params.base.soundThread=16;
        params.datacount=1;
        params.data=&field;
        lastError=sceUtilityOskInitStart(&params);
        if (lastError<0) return false;
        target=&name;
        return true;
    }

    void draw(unsigned frameStep=1) {
        switch (sceUtilityOskGetStatus()) {
        case PSP_UTILITY_DIALOG_VISIBLE:
            noteError(sceUtilityOskUpdate(frameStep));
            break;
        case PSP_UTILITY_DIALOG_QUIT:
            noteError(sceUtilityOskShutdownStart());
            break;
        case PSP_UTILITY_DIALOG_NONE:
            if (field.result==PSP_UTILITY_OSK_RESULT_CHANGED) {
                ProfileName name{};
                decode(output,name);
                if (!name[0] || Profiles::validName(name)) *target=name;
            }
            target=nullptr;
            break;
        default:
            break;
        }
    }

private:
    void noteError(int result) {
        if (result<0 && !lastError) lastError=result;
    }

    template<std::size_t N>
    static void encode(const char* source,std::array<unsigned short,N>& output) {
        output={};
        for (unsigned i=0;*source && i+1<N;++i) {
            const unsigned a=static_cast<unsigned char>(*source++);
            unsigned ch=a;
            if (a>=0xc2 && a<0xe0 && *source) {
                ch=((a&31)<<6)|(static_cast<unsigned char>(*source++)&63);
            } else if (a>=0xe0 && a<0xf0 && *source && source[1]) {
                const unsigned b=static_cast<unsigned char>(*source++);
                const unsigned c=static_cast<unsigned char>(*source++);
                ch=((a&15)<<12)|((b&63)<<6)|(c&63);
            }
            output[i]=static_cast<unsigned short>(ch);
        }
    }

    static void decode(const std::array<unsigned short,ProfileNameLength+1>& chars,ProfileName& name) {
        unsigned pos=0;
        for (unsigned i=0;i<ProfileNameLength && chars[i];++i) {
            const unsigned ch=chars[i];
            if (ch<0x80) name[pos++]=static_cast<char>(ch);
            else if (ch<0x800) {
                name[pos++]=static_cast<char>(0xc0|(ch>>6));
                name[pos++]=static_cast<char>(0x80|(ch&63));
            } else {
                name[pos++]=static_cast<char>(0xe0|(ch>>12));
                name[pos++]=static_cast<char>(0x80|((ch>>6)&63));
                name[pos++]=static_cast<char>(0x80|(ch&63));
            }
        }
    }

    SceUtilityOskData field{};
    SceUtilityOskParams params{};
    ProfileName* target=nullptr;
    int lastError=0;
    std::array<unsigned short,ProfileNameLength+1> input{},output{};
    std::array<unsigned short,48> description{};
};
