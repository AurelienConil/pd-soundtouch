# CLAUDE.md — Pure Data externals (C/C++)

## Projet : pitchshift~

External Pure Data utilisant la lib **SoundTouch** (dans `soundtouch/`) pour le pitch shifting temps réel.

- Fichier principal : `pitchshift_tilde.cpp` (C++ car SoundTouch est C++)
- L'external doit exposer `pitchshift_tilde_setup()` en `extern "C"`
- SoundTouch est compilé depuis les sources dans `soundtouch/source/SoundTouch/`

### Pattern SoundTouch (tiré de inspiration.cpp)

```cpp
soundtouch::SoundTouch stouch_;

// Init (dans constructeur / new)
stouch_.setChannels(1);
stouch_.setSampleRate(sys_getsr() ? sys_getsr() : 44100);
stouch_.setTempo(1);
stouch_.setRate(1);
stouch_.setPitch(1);
stouch_.setSetting(SETTING_USE_QUICKSEEK, 0);
stouch_.setSetting(SETTING_USE_AA_FILTER, 0);
stouch_.setSetting(SETTING_SEQUENCE_MS, 40);
stouch_.setSetting(SETTING_SEEKWINDOW_MS, 20);
stouch_.setSetting(SETTING_OVERLAP_MS, 10);

// Changer le pitch (en demi-tons)
stouch_.setPitchSemiTones(semitones);  // float, ex: -12..+12

// DSP perform (pipeline FIFO avec latence)
stouch_.putSamples(fin, n);
auto nsamp = stouch_.receiveSamples(fout, n);
// nsamp peut être < n au démarrage (latence initiale) → zéroer le reste
for (size_t i = nsamp; i < n; i++) fout[i] = 0;
```

**Attention :** SoundTouch introduit une latence initiale (~quelques centaines de samples). Les premiers blocs en sortie peuvent être silencieux.

**Attention inspiration.cpp :** utilise le framework ceammc (non disponible ici). Ne pas copier les classes `SoundExternal`, `BoolProperty`, etc. S'en inspirer uniquement pour l'usage de SoundTouch.

## Structure d'un external

### Fichiers
- Un fichier `.cpp` par external (SoundTouch impose C++). Nom = nom de l'objet Pd (ex: `pitchshift_tilde.cpp`).
- Fichier de tête : commentaire une ligne décrivant le rôle de l'external.

### Includes obligatoires
```c
#include "m_pd.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
```

### Squelette minimal (ordre des sections)
1. Includes
2. Constantes globales (`#define`, `#ifndef M_PI`)
3. Déclaration du pointeur de classe : `static t_class *myobj_tilde_class;`
4. Struct : `typedef struct _myobj_tilde { ... } t_myobj_tilde;`
5. Forward declarations des fonctions statiques
6. Fonctions helpers statiques
7. Handlers de messages (`void myobj_tilde_mymsg(...)`)
8. `void *myobj_tilde_new(...)` — constructeur
9. `static t_int *myobj_tilde_perform(t_int *w)` — boucle DSP
10. `void myobj_tilde_dsp(...)` — enregistrement DSP
11. `void myobj_tilde_setup(void)` — enregistrement classe + méthodes

### Struct : champs obligatoires
```c
typedef struct _myobj_tilde {
    t_object x_obj;   // DOIT être le premier champ
    t_float f;        // dummy float pour CLASS_MAINSIGNALIN
    // outlets
    t_outlet *x_audio_out;
    // ... paramètres métier ...
} t_myobj_tilde;
```

### Outlets
- **L'outlet audio doit être créé en premier** (`outlet_new(..., &s_signal)`).
- Ensuite les outlets de contrôle (bang, anything...).

### Constructeur `_new`
- Appeler `pd_new(myobj_tilde_class)`.
- Créer tous les outlets dans l'ordre (audio en premier).
- Initialiser tous les champs à des valeurs sûres (NULL, 0, valeurs par défaut).
- Retourner `(void *)x`.

### Boucle DSP `_perform`
- Signature : `static t_int *myobj_tilde_perform(t_int *w)`
- `w[1]` = pointeur objet, `w[2]` = buffer audio, `w[3]` = n (taille bloc)
- Retourner `(w + 4)` (ou `w + 2 + nb_args`).
- Toujours écrire `out[i] = 0` comme valeur de sécurité si l'objet est inactif.

### Enregistrement DSP `_dsp`
```c
void myobj_tilde_dsp(t_myobj_tilde *x, t_signal **sp) {
    dsp_add(myobj_tilde_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}
```
- `sp[0]` = inlet audio, `sp[1]` = outlet audio (si séparés).

### Setup `_setup`
```c
void myobj_tilde_setup(void) {
    myobj_tilde_class = class_new(gensym("myobj~"),
        (t_newmethod)myobj_tilde_new,
        0, sizeof(t_myobj_tilde), CLASS_DEFAULT, 0);
    class_addmethod(..., gensym("dsp"), A_CANT, 0);
    // autres méthodes...
    CLASS_MAINSIGNALIN(myobj_tilde_class, t_myobj_tilde, f);
}
```
- `CLASS_MAINSIGNALIN` active l'inlet audio par défaut (utilise le champ `f`).

## Messages Pd
- `A_FLOAT` pour un paramètre flottant simple.
- `A_GIMME` pour des listes (argc/argv).
- `A_CANT` pour `dsp` (réservé).
- Toujours clamper les valeurs reçues en message dans des bornes raisonnables.

## Gestion mémoire
- `malloc` / `realloc` / `free` — pas de GC.
- Initialiser les pointeurs à `NULL` dans `_new`.
- Libérer avant de réallouer : `if (x->buf) { free(x->buf); x->buf = NULL; }`.
- Vérifier les retours de `malloc` (au minimum ne pas déréférencer si NULL).

## Audio
- Utiliser `t_sample` pour les buffers audio dans `_perform`.
- `t_float` pour les flottants de paramètres dans la struct.
- Privilégier `double` pour les positions/accumulateurs longue durée (évite le drift).
- Crossfade equal-power : `sinf(theta)` / `cosf(theta)` avec `theta = pi/2 * t`.

## Sorties non-audio (messages)
```c
// Bang
outlet_bang(x->x_bang_out);

// Float
t_atom a; SETFLOAT(&a, val); outlet_float(x->x_float_out, val);

// Message avec tag
t_atom args[2]; SETFLOAT(&args[0], v1); SETFLOAT(&args[1], v2);
outlet_anything(x->x_print_out, gensym("info"), 2, args);
```

## Conventions de nommage
- Classe : `myobj_tilde_class` (static global)
- Struct : `t_myobj_tilde`, `_myobj_tilde`
- Fonctions : `myobj_tilde_<action>` (ex: `myobj_tilde_perform`, `myobj_tilde_new`)
- Champs struct : préfixe `x_` (ex: `x_audio_out`, `x_playing`)
- Helpers internes : `static`, sans préfixe objet (ex: `find_zero_crossing_near`)
