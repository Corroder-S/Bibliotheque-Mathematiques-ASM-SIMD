# Bibliothèque Mathématique ASM / SIMD

**Projet** : GTech 3 — Spécialisation Moteur
**Auteurs** : Sacha Brossard et Léo Grognet
**Période** : 21 septembre — 9 octobre 2026

---

## Compilation

**Prérequis**
- Visual Studio 2022 (MSVC 19.4x)
- CMake 3.20+
- ml64.exe (inclus avec MSVC, pour le fichier ASM)
- Connexion internet au premier build (FetchContent télécharge Google Test et Google Benchmark)

**Via Visual Studio**

Ouvrir le dossier racine directement dans Visual Studio (`Fichier > Ouvrir > Dossier`). Visual Studio détecte le `CMakeLists.txt` et configure automatiquement les cibles. Sélectionner la configuration `x64-Release` pour les benchmarks.

**Via ligne de commande**

```bat
cmake -B out/build/x64-Release -S . -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build out/build/x64-Release
```

Les binaires produits :
- `MathStarterTests.exe` — suite de tests unitaires
- `MathStarterBenchmark.exe` — application console de benchmark
- `MathStarterDemo.exe` — application de démonstration

---

## Lancement des tests

```bat
cd out/build/x64-Release
ctest --output-on-failure
```

Ou directement :

```bat
Tests\MathStarterTests.exe
```

La suite couvre : Vec3, Vec4, Matrix3x3, Matrix4x4, traitements par lots (Vec3Batch, Mat4Batch), fonction ASM, détection CPU.

---

## Lancement des benchmarks

Exécuter en **Release x64, sans débogueur**. Un menu interactif permet de sélectionner le traitement à mesurer :

```bat
MatStarter.Benchmark\MathStarterBenchmark.exe
```

```
1. DotProduct AoS (Ref + SSE)
2. DotProduct SoA (Ref + SSE)
3. Normalize AoS (Ref + SSE)
4. DotProduct ASM
5. TransformPoint (Ref + SSE)
6. CrossProduct SoA (Ref + SSE)
7. Conversion AoS -> SoA
8. Matrix Multiplication (Ref + SSE)
9. Tout lancer
0. Quitter
```

Avant le menu, un prompt de seed permet de choisir la graine utilisée pour générer les données (saisie libre ou preset : 42 / 123 / 1337 / 99999 / 3735928559 — défaut : 42). La seed est affichée avant chaque lancement pour garantir la reproductibilité.

Chaque benchmark tourne sur 3 tailles (64, 4096, 1 048 576 éléments), 5 répétitions. Les résultats bruts sont joints au dépôt.

---

## Conventions de l'API

**Types principaux**

```cpp
using Vec3f = Vec3<float>;

struct Vec3SoA {
    std::vector<float> x, y, z;
};
```

**Namespaces**

| Namespace | Rôle |
|---|---|
| `Maths::Ref` | Implémentation C++ scalaire de référence |
| `Maths::SSE` | Implémentation SIMD explicite (intrinsics SSE/SSE2) |
| `Maths::ASM` | Implémentation en assembleur x64 (MASM) |

**Chargements mémoire**

Toujours `_mm_loadu_ps` / `_mm_storeu_ps` (non aligné), jamais `_mm_load_ps`. `Vec3f` fait 12 octets — pas un multiple de 16.

**Vecteur nul ou non fini**

`NormalizeBatch_AOS` : un vecteur nul ou avec une composante NaN / infini produit `Vec3f::Zero` en sortie, sans exception.

**Stockage des matrices**

`Matrix4x4` est stockée par lignes (row-major). `TransformPoint` applique `M * [x, y, z, 1]ᵀ` (coordonnée homogène w = 1, sans division perspective). La dernière ligne doit être `[0, 0, 0, 1]` (matrice affine).

**Tolérance numérique**

```cpp
absoluteTolerance = 64  * epsilon<float>   // ~7.6e-6
relativeTolerance = 128 * epsilon<float>   // ~1.5e-5
tolerance = absoluteTolerance + relativeTolerance * |expected|
```

---

## Fonctionnalités réalisées

| Traitement | Ref | SSE | ASM | Obligatoire |
|---|---|---|---|---|
| DotProductBatch AoS | ✓ | ✓ | ✓ | ✓ |
| DotProductBatch SoA | ✓ | ✓ | — | ✓ |
| NormalizeBatch AoS | ✓ | ✓ | — | ✓ |
| TransformPointBatch | ✓ | ✓ | — | ✓ |
| ConvertToSOA | ✓ | — | — | ✓ |
| CrossProductBatch SoA | ✓ | ✓ | — | — |
| MultiplyMatrixBatch | ✓ | ✓ | — | — |

- Comparaison AoS / SoA sur le produit scalaire, coût de conversion mesuré séparément
- Fonction ASM x64 (MASM) : `ASM_DotProductBatch_AOS`, convention d'appel Windows x64
- Deux extraits de désassemblage commentés (voir rapport technique)
- Profil CPU sur le produit scalaire AoS (Visual Studio Utilisation de l'UC)

---

## Limites connues

- `NormalizeBatch_AOS` SSE n'a pas la protection contre le dépassement de capacité pour des valeurs proches de `FLT_MAX` (contrairement à la version Ref).
- `TransformPointBatch` SSE traite un point par itération (pas 4 en parallèle) — passer en SoA permettrait un gain plus important.
- Notre version SSE SoA du produit scalaire recharge les pointeurs des `std::vector` à chaque tour de boucle, ce qui explique qu'elle soit plus lente que la version Ref auto-vectorisée par MSVC.
- La fonction ASM est plus lente que la référence C++ (1.16x à 1.28x) — analyse détaillée dans le rapport.
- SSE/SSE2 uniquement, pas d'AVX.
- Toutes les mesures proviennent d'une seule machine (12 x 4691 MHz, L3 32 MiB).

---

## Sources utilisées

- [Intel Intrinsics Guide](https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html)
- [Google Benchmark](https://github.com/google/benchmark) — guide méthodologique et exemples
- [Google Test](https://github.com/google/googletest)
- Documentation MSVC : `/arch:SSE2`, `/O2`, convention d'appel x64, fenêtre Code Machine
- Visual Studio — outil Utilisation de l'UC (profiler CPU)
- MASM x64 : intégration d'un fichier `.asm` dans un projet CMake/MSVC
