/* pitchshift_tilde.cpp - Pure Data external: real-time mono pitch shifter via SoundTouch */

#include <soundtouch/SoundTouch.h>

extern "C" {
#include "m_pd.h"
}

#include <cstring>

#define PITCH_MIN  -36.0f
#define PITCH_MAX   36.0f

static t_class *pitchshift_tilde_class;

typedef struct _pitchshift_tilde {
    t_object x_obj;
    t_float  f;              // dummy float for CLASS_MAINSIGNALIN

    t_outlet *x_audio_out;

    float pitch_semitones;
    int   quickseek;         // 1 = fast (lower quality), 0 = normal
    int   antialias;         // 1 = enabled, 0 = disabled (saves CPU)

    soundtouch::SoundTouch *stouch;
} t_pitchshift_tilde;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static void apply_settings(t_pitchshift_tilde *x)
{
    x->stouch->setSetting(SETTING_USE_QUICKSEEK,  x->quickseek);
    x->stouch->setSetting(SETTING_USE_AA_FILTER,  x->antialias);
    // Tuned for low latency / low CPU on embedded targets
    x->stouch->setSetting(SETTING_SEQUENCE_MS,   40);
    x->stouch->setSetting(SETTING_SEEKWINDOW_MS, 15);
    x->stouch->setSetting(SETTING_OVERLAP_MS,     8);
}

// ---------------------------------------------------------------------------
// Message handlers
// ---------------------------------------------------------------------------

extern "C" void pitchshift_tilde_pitch(t_pitchshift_tilde *x, t_floatarg f)
{
    if (f < PITCH_MIN) f = PITCH_MIN;
    if (f > PITCH_MAX) f = PITCH_MAX;
    x->pitch_semitones = f;
    x->stouch->setPitchSemiTones(f);
}

// Message `quality 0|1` : 0 = performance (quickseek, no AA), 1 = quality
extern "C" void pitchshift_tilde_quality(t_pitchshift_tilde *x, t_floatarg f)
{
    int q = (f > 0.5f) ? 1 : 0;
    x->quickseek = q ? 0 : 1;
    x->antialias = q ? 1 : 0;
    apply_settings(x);
}

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

extern "C" void pitchshift_tilde_free(t_pitchshift_tilde *x)
{
    delete x->stouch;
}

extern "C" void *pitchshift_tilde_new(t_floatarg pitch)
{
    t_pitchshift_tilde *x = (t_pitchshift_tilde *)pd_new(pitchshift_tilde_class);

    x->x_audio_out    = outlet_new(&x->x_obj, &s_signal);
    x->pitch_semitones = 0.0f;
    x->quickseek       = 1;   // performance mode by default
    x->antialias       = 0;

    x->stouch = new soundtouch::SoundTouch();
    x->stouch->setChannels(1);
    x->stouch->setSampleRate(sys_getsr() > 0 ? (unsigned int)sys_getsr() : 44100);
    x->stouch->setTempo(1.0);
    x->stouch->setRate(1.0);
    apply_settings(x);

    if (pitch != 0.0f)
        pitchshift_tilde_pitch(x, pitch);
    else
        x->stouch->setPitchSemiTones(0.0f);

    return (void *)x;
}

// ---------------------------------------------------------------------------
// DSP perform
// ---------------------------------------------------------------------------

static t_int *pitchshift_tilde_perform(t_int *w)
{
    t_pitchshift_tilde *x = (t_pitchshift_tilde *)(w[1]);
    t_sample *in          = (t_sample *)(w[2]);
    t_sample *out         = (t_sample *)(w[3]);
    int       n           = (int)(w[4]);

    // SoundTouch works with float; t_sample is float on most Pd builds
    x->stouch->putSamples(in, (unsigned int)n);
    unsigned int received = x->stouch->receiveSamples(out, (unsigned int)n);

    // Zero-fill the initial latency gap
    if (received < (unsigned int)n)
        memset(out + received, 0, sizeof(t_sample) * (n - received));

    return (w + 5);
}

// ---------------------------------------------------------------------------
// DSP registration
// ---------------------------------------------------------------------------

extern "C" void pitchshift_tilde_dsp(t_pitchshift_tilde *x, t_signal **sp)
{
    // Update sample rate if Pd's changed (e.g. after patch reload)
    x->stouch->setSampleRate((unsigned int)sp[0]->s_sr);
    dsp_add(pitchshift_tilde_perform, 4,
            x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------

extern "C" void pitchshift_tilde_setup(void)
{
    pitchshift_tilde_class = class_new(gensym("pitchshift~"),
        (t_newmethod)pitchshift_tilde_new,
        (t_method)pitchshift_tilde_free,
        sizeof(t_pitchshift_tilde),
        CLASS_DEFAULT,
        A_DEFFLOAT, 0);   // optional creation arg: initial pitch in semitones

    class_addmethod(pitchshift_tilde_class,
        (t_method)pitchshift_tilde_dsp,     gensym("dsp"),     A_CANT,  0);
    class_addmethod(pitchshift_tilde_class,
        (t_method)pitchshift_tilde_pitch,   gensym("pitch"),   A_FLOAT, 0);
    class_addmethod(pitchshift_tilde_class,
        (t_method)pitchshift_tilde_quality, gensym("quality"), A_FLOAT, 0);

    CLASS_MAINSIGNALIN(pitchshift_tilde_class, t_pitchshift_tilde, f);
}
