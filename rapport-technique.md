# Rapport technique ASM / SIMD, Bibliothèque mathématique

**Projet** : GTech 3 Spécialisation Moteur
**Module** : ASM / SIMD Bibliothèque mathématique
**Auteurs** : Sacha Brossard et Léo Grognet

---

## 1. État initial de la bibliothèque

La bibliothèque de référence fournie (`Vec3.h`, `Vec4.h`) implémente des vecteurs génériques `Vec3<T>` / `Vec4<T>` (template sur `float`/`double`), en C++20 et sans SIMD. C'est le socle scalaire de comparaison.

Conventions mathématiques et stockage:

-   Stockage des matrices : Organisation "Row-Major" (les lignes sont contiguës en mémoire).
    
-   Vecteurs : Traitement sous forme de vecteurs colonnes (result = Matrix x Vector).
    
-   Ordre de composition : A x B applique B puis A. Les angles sont en radians.

Observations :
- Les opérations de base (`Dot`, `Cross`, `Normalize`, `Magnitude`, `Lerp`, `Min`/`Max`) sont correctes et vérifiées par les tests unitaires (Google Test).
- `Normalize()` lance un `std::domain_error` pour un vecteur nul ou des composantes non finies.
- `operator==` compare les float par égalité stricte. C'est à éviter pour valider des résultats numériques car, une fois stockés en binaire, les float peuvent être arrondis. Nos tests utilisent donc une comparaison avec tolérance (`Near`, `VectorNear` dans `TestHelpers.h`).
- Aucune modification du socle de référence n'a été nécessaire. On l'a seulement complété avec les traitements par lots (`Vec3Batch.h` pour les vecteurs et `Mat4Batch.h` pour les matrices).

---

## 2. Organisation mémoire

### 2.1 AoS (Array of Structures)

Format par défaut de la bibliothèque : un tableau de `Vec3f`, chaque élément regroupant `x`, `y`, `z` les uns à la suite des autres en mémoire.

```
a[0].x  a[0].y  a[0].z  a[1].x  a[1].y  a[1].z  a[2].x ...
```

### 2.2 SoA (Structure of Arrays)

Format alternatif, implémenté pour le produit scalaire (`Vec3SoA`) :

```cpp
struct Vec3SoA
{
    std::vector<float> x;  // tous les x, contigus
    std::vector<float> y;  // tous les y, contigus
    std::vector<float> z;  // tous les z, contigus
};
```

La conversion AoS vers SoA (fonction `ConvertToSOA`) a un coût propre, qu'on a mesuré séparément (voir 4.3).

### 2.3 Alignement

Aucune contrainte d'alignement forcée (`alignas`) sur `Vec3f` (12 octets, donc pas un multiple de 16). Les chargements SIMD utilisent toujours des instructions non alignées (`_mm_loadu_ps` / `_mm_storeu_ps`), jamais `_mm_load_ps` / `_mm_store_ps`, pour que le code reste valide quel que soit l'alignement du buffer donné par l'appelant.

---

## 3. Traitements étudiés

### 3.1 Produit scalaire par lot (`DotProductBatch`)

Signatures :
```cpp
void DotProductBatch_AOS(const Vec3f* a, const Vec3f* b, float* result, size_t count);
void DotProductBatch_SOA(const Vec3SoA& a, const Vec3SoA& b, float* result, size_t count);
```

#### Version AoS : première version abandonnée

Notre première version traitait 4 vecteurs à la fois. Pour cela on rassemblait les x, y et z de 4 `Vec3f` dans trois registres avec `_mm_set_ps`. Avec l'application console de benchmark, cette version était presque deux fois plus lente que la référence (ratio de 0.52 pour 100 000 éléments). Le désassemblage (voir 5.3, complément) montre pourquoi : il faut une trentaine d'instructions juste pour reconstruire les registres. On a donc changé d'approche.

#### Version AoS finale :

On charge un seul `Vec3f` par itération avec `_mm_loadu_ps`, puis on fait une réduction horizontale avec `shuffle` :

```cpp
__m128 vA = _mm_loadu_ps(&a[i].x);
__m128 vB = _mm_loadu_ps(&b[i].x);
__m128 vMul = _mm_mul_ps(vA, vB);
__m128 vY = _mm_shuffle_ps(vMul, vMul, _MM_SHUFFLE(1,1,1,1));
__m128 vZ = _mm_shuffle_ps(vMul, vMul, _MM_SHUFFLE(2,2,2,2));
__m128 vSum = _mm_add_ps(vMul, _mm_add_ps(vY, vZ));
result[i] = _mm_cvtss_f32(vSum);
```

`_mm_loadu_ps(&a[i].x)` lit 4 floats alors qu'un `Vec3f` n'en contient que 3, donc le 4e est celui du vecteur suivant. Pour le dernier élément du tableau, cette lecture sortirait du tableau. Pour éviter ça, le dernier élément est traité à part, en scalaire, et on ne lit jamais hors limites.

Le `shuffle` est nécessaire parce que `_mm_add_ps` additionne lane par lane entre deux registres, pas les lanes d'un même registre entre elles. Il faut donc recopier chaque composante dans toutes les lanes avant de pouvoir les sommer.

#### Version SoA : approche retenue

```cpp
__m128 ax = _mm_loadu_ps(&a.x[i]);   // 4 x consécutifs contigus en mémoire
__m128 ay = _mm_loadu_ps(&a.y[i]);
__m128 az = _mm_loadu_ps(&a.z[i]);
// pareil pour b
__m128 dot = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ax,bx), _mm_mul_ps(ay,by)), _mm_mul_ps(az,bz));
_mm_storeu_ps(&result[i], dot);
```

Ici il n'y a pas de réduction horizontale : chaque lane du registre `dot` correspond déjà à un vecteur différent du lot, et la somme se fait entre 3 registres différents, pas à l'intérieur d'un seul.

#### Gestion des tailles non multiples de 4

Une boucle principale avance par pas de 4 (`i + simdWidth <= count`), puis une boucle de reste scalaire s'occupe des éléments restants (0 à 3). Le lot vide est géré naturellement, car aucune des deux boucles ne s'exécute si `count == 0`.

### 3.2 Normalisation par lot (`NormalizeBatch`)

Signature :
```cpp
void NormalizeBatch_AOS(const Vec3f* input, Vec3f* output, size_t count);
```

#### Convention retenue pour le vecteur nul ou non fini

`Vec3::Normalize()` lance une exception sur un vecteur nul. Ce n'est pas utilisable dans un calcul SIMD, car on ne peut pas "lancer" pour une seule lane sur 4.

Décision, appliquée de la même façon à la référence et au SIMD : un vecteur nul ou avec une composante non finie (NaN ou infini) donne `Vec3f::Zero` en sortie, sans exception. Les raisons :
- Ça marche avec un masque SIMD sans branchement (`_mm_cmpeq_ps` et `_mm_andnot_ps`).
- C'est déterministe et facile à tester (on compare directement à `Vec3f::Zero`).
- Ça évite de propager des NaN dans le reste du programme.
- C'est le comportement de plusieurs moteurs (par exemple `Vector3.normalized` dans Unity).

#### Implémentation SIMD

On rassemble 4 vecteurs (`_mm_set_ps`), on calcule la norme au carré, puis un masque `zeroMask` repère les lanes nulles. La division est protégée : on remplace la norme nulle par 1 pour ne pas diviser par zéro, et le résultat est ensuite écrasé par le masque. La sortie est écrite à la main (buffer temporaire aligné puis recopie scalaire), car elle reste en AoS.

Limite connue : la version scalaire de référence divise d'abord par la plus grande composante pour éviter un dépassement de capacité sur de très grandes valeurs. La version SIMD ne fait pas cette protection. Ce n'est pas gênant pour l'usage visé (vecteurs de jeu, valeurs raisonnables), mais il faut le savoir.

### 3.3 Transformation de points par lot (`TransformPointBatch`)

Ce traitement a été réalisé par Léo.

Signature :
```cpp
void TransformPointBatch(const Matrix4x4<>& transform, const Vec3<>* points,
                         Vec3<>* outPoints, size_t count);
```

Contexte : `Matrix4x4` est stockée par lignes (row-major). `TransformPoint` applique `matrice * [x, y, z, 1]` (point homogène avec `w = 1`, sans division perspective), puis le résultat est tronqué en `Vec3`.

#### Version Référence

Boucle scalaire simple qui appelle `transform.TransformPoint(points[i])` pour chaque point.
MSVC ne semble pas l'auto-vectoriser, car cet appel de fonction n'est pas inliné.

#### Version SSE

Pourquoi la transposition ? La matrice est stockée par lignes : `values[row]` est une ligne. Après `Transpose()`, `trans.values[0]` contient la colonne 0 de la matrice d'origine, c'est à dire les 4 coefficients qui multiplient la composante x du point. On peut donc charger chaque colonne dans un registre `__m128`, puis la multiplier par la composante du point recopiée sur les 4 lanes avec `_mm_set1_ps` :

```cpp
const Matrix4x4<> trans = transform.Transpose();
__m128 pX = _mm_loadu_ps(trans.values[0]); // colonne X de la matrice d'origine
__m128 pY = _mm_loadu_ps(trans.values[1]); // colonne Y
__m128 pZ = _mm_loadu_ps(trans.values[2]); // colonne Z
__m128 pW = _mm_loadu_ps(trans.values[3]); // colonne W (translation et 1)

for (size_t i = 0; i < count; ++i)
{
    __m128 bx = _mm_mul_ps(pX, _mm_set1_ps(points[i].x)); // colonne X * px
    __m128 by = _mm_mul_ps(pY, _mm_set1_ps(points[i].y)); // colonne Y * py
    __m128 bz = _mm_mul_ps(pZ, _mm_set1_ps(points[i].z)); // colonne Z * pz
    __m128 result = _mm_add_ps(_mm_add_ps(bx, by), _mm_add_ps(bz, pW));
    float tmp[4];
    _mm_storeu_ps(tmp, result);
    outPoints[i] = {tmp[0], tmp[1], tmp[2]};
}
```

Écriture à la main : le résultat est d'abord stocké dans un petit buffer `float tmp[4]`, puis les 3 premières valeurs sont recopiées dans `outPoints[i]`. C'est obligatoire parce que le format de sortie reste en AoS (`Vec3`, 12 octets).

Précondition : la dernière ligne de la matrice doit être [0, 0, 0, 1] (matrice affine). Un `std::domain_error` est lancé si ce n'est pas le cas, avant d'entrer dans la boucle de calcul.

Limite : cette version ne traite qu'un seul point par itération (les 4 composantes d'un même point dans un registre, et pas 4 points en parallèle). Pour avoir un gain plus important, il faudrait traiter 4 points en même temps, ce qui demanderait de passer les points en entrée au format SoA.

---

## 4. Protocole de benchmark

### 4.1 Environnement

- CPU : 12 X 4691 MHz (valeur donnée par Google Benchmark)
- Caches : L1 données 32 KiB, L2 1 MiB, L3 32 MiB
- Compilateur : MSVC 19.44 (Visual Studio 2022)
- Configuration : Release x64
- Jeu d'instructions : SSE/SSE2 (`/arch:SSE2` ou défaut x64)
- Outil : Google Benchmark, 3 tailles de lot (64, 4096 et 1 048 576 éléments), 5 répétitions
  par mesure
- Données générées hors chronométrage avec une graine fixe (42), et `DoNotOptimize` sur le résultat pour empêcher le compilateur de supprimer le calcul.
- Les sorties brutes de Google Benchmark (5 répétitions, moyenne, médiane, écart-type et cv) sont jointes au dépôt avec ce rapport.

Dans les tableaux on donne la médiane des 5 répétitions du temps réel (colonne Time de Google Benchmark), en nanosecondes. La dispersion est donnée par le coefficient de variation (cv) des 5 répétitions. Le ratio est le temps de la référence divisé par le temps de la version SIMD : plus il est grand, plus le SIMD est rapide, et en dessous de 1 le SIMD est plus lent que la référence.

Pour situer les tailles par rapport aux caches : avec 64 éléments les données tiennent dans le L1. Avec 4096 éléments, deux tableaux de `Vec3f` et le résultat font environ 112 KiB, donc ça dépasse le L1 mais ça tient dans le L2. Avec 1 048 576 éléments, ça fait environ 28 MiB, donc on dépasse le L2 et on approche la taille du L3.

### 4.2 Résultats : DotProduct

Temps en ns.

| Taille | Ref AoS | SSE AoS | Ratio AoS | Ref SoA | SSE SoA | Ratio SoA |
|---|---|---|---|---|---|---|
| 64 | 37.7 | 23.9 | 1.58x | 11.2 | 15.4 | 0.73x |
| 4096 | 2 329 | 1 442 | 1.62x | 701 | 1 099 | 0.64x |
| 1 048 576 | 714 661 | 531 587 | 1.34x | 370 504 | 447 205 | 0.83x |

Dispersion : pour l'AoS, le cv reste entre 0.28 % et 2.32 %. Pour le SoA il est sous 1.4 % à 64 et 4096 éléments, mais il monte à 4.76 % (Ref) et 6.55 % (SSE) à 1 048 576 éléments. Le ratio de 0.83 pour la plus grande taille est donc moins précis que les autres.

Observations :
- La version SSE AoS apporte un gain de 1.34x à 1.62x, qui diminue quand la taille augmente. Pour chaque vecteur on lit 6 floats et on en écrit 1. Le traitement est donc plutôt limité par la mémoire que par le calcul. Quand les données sortent des caches, la mémoire devient la limite. C'est cohérent avec ce qu'on observe, même si on ne l'a pas mesuré directement.
- La version SoA de référence (scalaire, sans intrinsics) est la plus rapide de toutes les versions. Au désassemblage, MSVC l'a auto-vectorisée en SSE avec les mêmes instructions que notre version manuelle (voir 5.3). Notre version SSE SoA est même plus lente que cette référence (ratio entre 0.64 et 0.83). On pense que c'est dû aux six pointeurs rechargés à chaque tour de boucle dans notre version (détail en 5.3).
- Si on compare AoS et SoA avec la référence seule, le SoA est 3.4x plus rapide à 64 éléments, 3.3x à 4096 et 1.9x à 1 048 576. Avec le SSE, l'écart est plus petit : 1.55x, 1.31x et 1.19x. Le format mémoire compte donc beaucoup, surtout parce qu'il permet au compilateur de vectoriser tout seul. Par contre ces chiffres ne comptent pas la conversion, qui est vue juste après.
- Les résultats de la fonction ASM sont donnés dans la partie 8.

### 4.3 Coût de conversion AoS vers SoA

Question : est-ce que ça vaut le coup de convertir en SoA avant de calculer, si les données arrivent en AoS ? On compare le coût total (conversion et calcul SSE SoA) au coût de l'alternative directe (calcul SSE AoS sans conversion).

Le benchmark `ConvertToSOA` convertit un seul tableau (120 ns pour 64 éléments, 2 846 ns pour 4096 et 3 100 866 ns pour 1 048 576). Comme le produit scalaire a deux tableaux en entrée, on a doublé ces valeurs pour la colonne conversion. Ce n'est donc pas une mesure directe des deux conversions ensemble. De plus, cette mesure comprend l'allocation des trois `std::vector` à chaque itération.

Temps en ns.

| Taille | Conversion (a et b) | SSE SoA | Total SoA | SSE AoS | Ratio (Total SoA / SSE AoS) |
|---|---|---|---|---|---|
| 64 | 240 | 15.4 | 255.4 | 23.9 | 10.7 |
| 4096 | 5 692 | 1 099 | 6 791 | 1 442 | 4.7 |
| 1 048 576 | 6 201 732 | 447 205 | 6 648 937 | 531 587 | 12.5 |

Ici le ratio se lit à l'envers par rapport aux autres : il indique combien de fois le SoA avec conversion est plus lent que le SSE AoS direct.

Pour un usage ponctuel (une seule passe sur les données), le SoA est 4.7 à 12.5 fois plus lent que de rester en AoS. La conversion coûte à elle seule entre 5 et 16 fois le temps du calcul SoA. Le gain du calcul (SSE AoS moins SSE SoA) est petit par rapport au coût de la conversion.

Si on réutilise les mêmes données converties pour plusieurs passes, la conversion est amortie. En divisant le coût de conversion par le gain par passe, il faudrait environ 29 passes pour 64 éléments, 17 passes pour 4096 et 74 passes pour 1 048 576 pour que le SoA devienne plus rentable que le SSE AoS. Pour la grande taille, le gain par passe est petit et la dispersion est de 6.55 %, donc c'est surtout un ordre de grandeur. Le format SoA n'est donc intéressant que si les données sont déjà en SoA, ou si elles servent pour plusieurs traitements successifs.

### 4.4 Résultats : Normalize

Temps en ns.

| Taille | Ref AoS | SSE AoS | Ratio |
|---|---|---|---|
| 64 | 220 | 72.0 | 3.06x |
| 4096 | 13 827 | 4 624 | 2.99x |
| 1 048 576 | 3 615 508 | 1 180 711 | 3.06x |

Dispersion : le cv est sous 1.4 % pour toutes les mesures (entre 0.29 % et 1.38 %).

Ici le gain est stable, autour de 3x, quelle que soit la taille. Ce n'est pas le même comportement que pour le produit scalaire, où le gain diminuait avec la taille. Notre hypothèse est que la normalisation fait plus de calcul par élément (une racine carrée et une division, qui sont des opérations coûteuses), donc elle est plutôt limitée par le calcul (compute-bound) et elle dépend moins de la mémoire. Le SIMD en profite alors mieux, même avec le rassemblage et l'écriture à la main.

Un autre indice va dans ce sens : à 1 048 576 éléments, la version SSE de Normalize traite environ 895 millions de vecteurs par seconde, contre environ 1.97 milliard pour le produit scalaire AoS SSE. Normalize est donc environ 2.2 fois plus lent par élément, ce qui est cohérent avec le coût de la racine carrée et de la division. Ça reste une hypothèse : on n'a pas fait de désassemblage ni de profil pour ce traitement.

### 4.5 Résultats : TransformPoint


| Taille | Ref | SSE | Ratio |
|---|---|---|---|
| 64 | 115 ns | 50.7 ns | 2.27x |
| 4096 | 7 367 ns | 3 015 ns | 2.44x |
| 1 048 576 | 1 931 158 ns | 785 929 ns | 2.46x |

Dispersion : le cv reste entre 0.44 % et 1.93 %.

Le gain est stable, entre 2.3x et 2.5x à toutes les tailles, comme pour Normalize et contrairement au produit scalaire AoS. On pense que c'est parce que le traitement reste limité par le calcul même à grande taille : pour chaque point il y a 3 multiplications et 3 additions vectorielles, et la transposition de la matrice n'est faite qu'une seule fois avant la boucle. De plus la référence est lente, car elle appelle `TransformPoint` pour chaque point et MSVC ne semble pas l'auto-vectoriser, ce qui explique un écart plus marqué que pour le produit scalaire AoS.

Comme on l'a écrit en 3.3, la version SSE ne traite qu'un point par itération. Passer à 4 points en parallèle (format SoA) pourrait donner un gain plus important, mais on ne l'a pas fait.

### 4.6 Résultats : Produit de matrice par lots

| Taille | Ref | SSE | Ratio |
|---|---|---|---|
| 64 | 715 ns | 184 ns | 3.88x |
| 4096 | 54 408 ns | 11 998 ns | 4.53x |
| 1 048 576 | 16 469 595 ns | 7 465 278 ns | 2.20x |

Ces opérations sont particulièrement lourdes à gérer étant donnée la taille de chaque matrice couplée à la taille des lots utilisés pour ce benchmark :
Une matrice occupe 64 octets, donc pour un traitement de 1 048 576 matrices, on arrive à 192 mo de données à traiter (matrices a, b et out, donc trois matrices à chaque fois).

Implémentation : On charge les 4 lignes de la matrice `b` (`b0` à `b3`) une seule fois dans des registres au début de la fonction. Ensuite, on prend les coefficients de la matrice `a` et on les duplique sur les voies d'un registre pour les appliquer aux valeurs de `b`, et on envoie le résultat dans la matrice `out`.


---

## 5. Analyse au profiler CPU

### 5.1 Protocole

Le profil a été fait avec l'outil "Utilisation de l'UC" de Visual Studio, sur `MathStarterBenchmark.exe`, avec le filtre `--benchmark_filter=DotProductBatch_AOS` pour ne profiler que le produit scalaire AoS (Ref et SSE).
	
Les pourcentages de ce profil ne représentent pas les performances Release. En Debug le compilateur n'optimise pas les intrinsics et n'inline rien. Seules les observations qualitatives sont utilisées.

### 5.2 Résultats : Top des fonctions

| Fonction | Temps CPU total | Temps CPU exclusif |
|---|---|---|
| `SSE::DotProductBatch_AOS` | 44,90 % | 44,70 % |
| `Ref::DotProductBatch_AOS` | 29,34 % | 19,70 % |
| `Vec3<float>::Dot` | 12,53 % | 12,43 % |
| `std::generate_canonical<...>` (génération aléatoire) | 12,23 % | 7,47 % |

Analyse :
- Dans ce profil Debug, `SSE::DotProductBatch_AOS` prend plus de temps que `Ref::DotProductBatch_AOS`. C'est l'inverse du résultat Release (1.34x à 1.62x plus rapide pour le SSE). On l'explique par le Debug : les intrinsics ne sont pas optimisées. Cette comparaison entre Ref et SSE ne doit donc pas être utilisée.
- `Vec3<float>::Dot` apparaît comme une fonction séparée dans le chemin d'appel de `Ref::DotProductBatch_AOS`. En Debug rien n'est inliné, donc ça ne prouve pas à lui seul que l'appel reste en Release. C'est le désassemblage Release (cas bloqué en 5.3) qui le montre.
- `std::generate_canonical` (génération des données aléatoires) apparaît avec un poids notable. Une partie du temps du profil correspond donc à la préparation des données et pas 			au traitement lui-même, alors que cette préparation est hors de la boucle chronométrée par Google Benchmark.

### 5.3 Désassemblage commenté

Les extraits ont été relevés sur le binaire Release x64, avec la fenêtre Code Machine de Visual Studio.

#### Extrait 1 : `SSE::DotProductBatch_SOA` (SIMD explicite), boucle principale

```asm
mov    rax, qword ptr [r11]         ; pointeur des x de a, rechargé
add    rcx, 4
movups xmm2, xmmword ptr [rax+r10*4]
mov    rax, qword ptr [rdx]         ; pointeur des x de b
movups xmm0, xmmword ptr [rax+r10*4]
mov    rax, qword ptr [r11+18h]     ; pointeur des y de a
mulps  xmm2, xmm0
movups xmm1, xmmword ptr [rax+r10*4]
mov    rax, qword ptr [rdx+18h]     ; pointeur des y de b
movups xmm0, xmmword ptr [rax+r10*4]
mov    rax, qword ptr [r11+30h]     ; pointeur des z de a
mulps  xmm1, xmm0
addps  xmm2, xmm1
movups xmm1, xmmword ptr [rax+r10*4]
mov    rax, qword ptr [rdx+30h]     ; pointeur des z de b
movups xmm0, xmmword ptr [rax+r10*4]
mulps  xmm1, xmm0
addps  xmm2, xmm1
movups xmmword ptr [r8+r10*4], xmm2 ; écriture des 4 résultats
add    r10, 4
cmp    rcx, r9
jbe    debut_de_boucle
```

Commentaire : 6 chargements de 4 floats (`movups`), 3 `mulps`, 2 `addps` et 1 écriture. Mais à chaque tour de boucle, le compilateur recharge depuis la mémoire les six pointeurs de données des `std::vector`.

#### Extrait 2 : `Ref::DotProductBatch_SOA` (référence optimisée, auto-vectorisée), boucle principale

```asm
movups xmm0, xmmword ptr [rdi+r10*4]
movups xmm2, xmmword ptr [rdx+r10*4]
movups xmm1, xmmword ptr [rsi+r10*4]
mulps  xmm2, xmm0
movups xmm0, xmmword ptr [rbp+r10*4]
mulps  xmm1, xmm0
movups xmm0, xmmword ptr [r15+r10*4]
addps  xmm2, xmm1
movups xmm1, xmmword ptr [r14+r10*4]
mulps  xmm1, xmm0
addps  xmm2, xmm1
movups xmmword ptr [r8+r10*4], xmm2
add    r10, 4
cmp    r10, rax
jb     debut_de_boucle
```

Commentaire : MSVC a auto-vectorisé cette boucle scalaire simple en SSE (registres `xmm`,  pas `ymm` donc pas d'AVX), avec les mêmes instructions de calcul que notre version manuelle. La différence est que les six pointeurs sont gardés dans des registres (`rdi`, `rdx`, `rsi`, `rbp`, `r15`, `r14`), sans les recharger à chaque tour. Avant cette boucle, le compilateur ajoute une série de vérifications au runtime pour s'assurer que les tableaux `a`, `b` et `result` ne se chevauchent pas (comparaisons `cmp` / `ja` / `jae`).

S'il y a un chevauchement, il utilise une version scalaire de secours. Une fois ces vérifications faites, il peut garder les pointeurs dans les registres. C'est un cas réussi d'auto-vectorisation, et ça explique en partie pourquoi notre version manuelle est un peu plus lente.

#### Complément : première version de `SSE::DotProductBatch_AOS` (rassemblement avec `_mm_set_ps`)

```asm
movss    xmm1, dword ptr [rcx+rax+10h]
movss    xmm0, dword ptr [rcx+rax-8]
unpcklps xmm2, xmm1
movlhps  xmm5, xmm2
...
mulps    xmm5, xmm3      ; calcul utile : ax*bx (4 lanes)
mulps    xmm4, xmm3      ; ay*by
addps    xmm5, xmm4
mulps    xmm4, xmm3      ; az*bz
addps    xmm5, xmm4
movups   xmmword ptr [r8+r11*4], xmm5   ; écriture du résultat
```

Commentaire : cet extrait vient de la première version, qui a été remplacée ensuite par la version avec `_mm_loadu_ps` et `shuffle` (voir 3.1). Avec `_mm_set_ps`, une trentaine d'instructions servent à reconstruire les registres de x, y et z à partir de 4 vecteurs AoS, pour seulement 5 instructions de calcul utile. Ça explique que cette version était plus lente que la référence.

#### Cas bloqué : `Ref::DotProductBatch_AOS`

```asm
call   Maths::Vec3<float>::Dot
movss  dword ptr [r14+rdi*4], xmm0
add    rbx, 0Ch
inc    rdi
cmp    rdi, rbp
jb     debut_de_boucle
```

Commentaire : cette version n'est pas auto-vectorisée. À chaque itération il y a un vrai `call` vers `Vec3<float>::Dot`, qui n'est pas inliné. On pense que cet appel empêche le compilateur de fusionner et de vectoriser la boucle, contrairement à la version SoA qui lit directement dans les tableaux.

---

## 6. Écarts numériques

La comparaison entre référence et SIMD n'utilise pas d'égalité stricte (`==`), car l'ordre des opérations flottantes n'est pas le même entre une boucle scalaire et une boucle vectorisée, ce qui donne des arrondis différents à la dernière décimale (ULP). Par exemple pour `DotProductBatch_AOS`, la référence calcule (x + y) + z alors que la version SSE calcule x + (y + z).

Tolérance appliquée (`TestHelpers::Near`, dans `TestHelpers.h`) :

```cpp
const T absoluteTolerance = T{64}  * std::numeric_limits<T>::epsilon(); // environ 7.6e-6 pour un float
const T relativeTolerance = T{128} * std::numeric_limits<T>::epsilon(); // environ 1.5e-5 pour un float
const T tolerance = absoluteTolerance + relativeTolerance * std::abs(expected);
```

C'est une tolérance mixte (absolue et relative). La partie absolue sert pour les valeurs proches de zéro, et la partie relative s'adapte à l'ordre de grandeur du résultat attendu. Un résultat NaN ou infini fait toujours échouer le test, même si l'écart calculé serait sous le seuil (c'est vérifié explicitement avec `std::isfinite`).

Cas particulier de `NormalizeBatch_AOS` : un vecteur nul ou contenant NaN ou infini donne `Vec3f::Zero` dans les deux implémentations (Ref et SSE). Ce cas est testé par comparaison directe, sans tolérance.

---

## 7. Limites connues

- `NormalizeBatch_AOS` en SIMD n'a pas la protection contre le dépassement de capacité de la version scalaire de référence, pour des valeurs extrêmes (proches de `FLT_MAX`).
- Pas de variante AVX (SSE/SSE2 uniquement, comme demandé pour le socle).
- Toutes les mesures viennent d'une seule machine, avec 5 répétitions. Pour le produit scalaire SoA à 1 048 576 éléments, la dispersion est de 5 à 7 %, donc les ratios de cette ligne sont moins précis.
- Le coût de conversion AoS vers SoA a été mesuré pour un seul tableau, puis doublé pour calculer le total. La mesure comprend aussi l'allocation des trois `std::vector`.
- Notre version SSE SoA recharge les pointeurs des `std::vector` à chaque tour de boucle. Une piste à tester : les copier dans des variables locales avant la boucle. On ne l'a pas encore essayé.
- Le profil CPU a été fait en Debug, donc seules les observations qualitatives sont utilisables.
- Pas de désassemblage ni de profil pour `NormalizeBatch` et `TransformPointBatch`.
- `TransformPointBatch` en SSE traite un seul point par itération (pas 4 en parallèle), donc le gain SIMD reste limité.
- La fonction ASM est plus lente que la référence C++ et que la version SSE (voir partie 8).

---

## 8. Fonction ASM x64

### 8.1 Convention d'appel Windows x64

Sur Windows x64, les 4 premiers arguments entiers ou pointeurs sont passés dans les registres `rcx`, `rdx`, `r8`, `r9`. La fonction `ASM_DotProductBatch_AOS` reçoit :

| Registre | Rôle |
|---|---|
| `rcx` | Pointeur vers le tableau `a` (Vec3f en AoS) |
| `rdx` | Pointeur vers le tableau `b` (Vec3f en AoS) |
| `r8` | Pointeur vers le tableau `result` (floats) |
| `r9` | Nombre d'éléments (`count`) |

### 8.2 Implémentation MASM

Un `Vec3f` occupe 12 octets consécutifs (x à l'offset 0, y à +4, z à +8). À chaque itération, on charge les 3 composantes de `a` et de `b` avec `movss` (qui met à zéro le reste du registre), on les regroupe avec deux `unpcklps`, on multiplie avec un seul `mulps`, puis on additionne les trois produits avec deux `shufps` et deux `addss`.

```asm
ASM_DotProductBatch_AOS PROC
    test r9, r9
    jz   done

loop_start:
    movss   xmm0, dword ptr [rcx]      ; a.x
    movss   xmm1, dword ptr [rcx+4]    ; a.y
    movss   xmm2, dword ptr [rcx+8]    ; a.z
    unpcklps xmm0, xmm1                ; xmm0 = [ax, ay, 0, 0]
    unpcklps xmm0, xmm2                ; xmm0 = [ax, az, ay, 0]

    movss   xmm1, dword ptr [rdx]      ; b.x
    movss   xmm2, dword ptr [rdx+4]    ; b.y
    movss   xmm3, dword ptr [rdx+8]    ; b.z
    unpcklps xmm1, xmm2                ; xmm1 = [bx, by, 0, 0]
    unpcklps xmm1, xmm3                ; xmm1 = [bx, bz, by, 0]

    mulps   xmm0, xmm1                 ; [ax*bx, az*bz, ay*by, 0]

    movaps  xmm1, xmm0
    shufps  xmm1, xmm1, 4Eh            ; xmm1 = [ay*by, 0, ax*bx, az*bz]
    addss   xmm0, xmm1                 ; lane 0 = ax*bx + ay*by

    movaps  xmm1, xmm0
    shufps  xmm1, xmm1, 11h            ; lane 0 de xmm1 = lane 1 de xmm0 = az*bz
    addss   xmm0, xmm1                 ; lane 0 = ax*bx + ay*by + az*bz

    movss   dword ptr [r8], xmm0       ; écriture du résultat

    add rcx, 12                        ; Vec3f suivant dans a (12 octets)
    add rdx, 12                        ; Vec3f suivant dans b
    add r8,  4                         ; float suivant dans result
    dec r9
    jnz loop_start

done:
    ret
ASM_DotProductBatch_AOS ENDP
```

À cause des deux `unpcklps`, l'ordre des lanes est [x, z, y, 0] et pas [x, y, z, 0]. Ça ne change pas le résultat, parce que `a` et `b` sont rangés dans le même ordre avant la multiplication, et qu'on additionne ensuite les trois produits.

### 8.3 Résultats

Produit scalaire AoS, temps en ns (médiane du temps réel), mesuré dans le même passage que Ref AoS et SSE AoS. Dans les deux dernières colonnes, un chiffre au dessus de 1 veut dire que l'ASM est plus lent.

| Taille | Ref AoS | SSE AoS | ASM | Temps ASM / Temps Ref | Temps ASM / Temps SSE |
|---|---|---|---|---|---|
| 64 | 37.7 | 23.9 | 47.2 | 1.25 | 1.97 |
| 4096 | 2 329 | 1 442 | 2 970 | 1.28 | 2.06 |
| 1 048 576 | 714 661 | 531 587 | 827 229 | 1.16 | 1.56 |

La dispersion est faible (cv entre 0.40 % et 1.60 %). Notre fonction ASM est donc 1.16 à 1.28 fois plus lente que la référence C++ optimisée par le compilateur, et 1.6 à 2.1 fois plus lente que la version SSE AoS.

### 8.4 Pourquoi elle est plus lente

Si on compte les instructions du code ci-dessus, une itération en contient 23 : 6 `movss` de chargement, 4 `unpcklps`, 1 `mulps`, 2 `movaps`, 2 `shufps`, 2 `addss`, 1 `movss` d'écriture, 3 `add` sur les pointeurs, et `dec` et `jnz`. Beaucoup de ces instructions servent seulement à reconstruire les registres et à réorganiser les lanes, et pas à calculer.

Par comparaison, la version SSE AoS charge un vecteur avec un seul `movups`. Et la fonction `Vec3::Dot` produite par le compilateur (vue au désassemblage) calcule le produit scalaire avec seulement 8 instructions scalaires (3 `movss`, 3 `mulss` et 2 `addss`). Notre code ASM fait donc plus de travail que la référence pour obtenir le même résultat.

Ce n'est pas le coût de l'appel de fonction qui explique l'écart. La boucle est à l'intérieur de la fonction ASM, il n'y a donc qu'un appel par lot. La référence C++, elle, fait un appel à `Vec3::Dot` pour chaque vecteur (voir 5.3), et elle reste quand même plus rapide.

### 8.5 Pistes d'amélioration

- Utiliser un `Vec3f` complété à 16 octets (padding) : on pourrait charger un vecteur avec un seul `movaps` au lieu de six `movss` et quatre `unpcklps`. Par contre ça change la taille de `Vec3f`, donc ça touche à la bibliothèque.
- Utiliser un format SoA en entrée : les tableaux `x[]`, `y[]`, `z[]` séparés permettent des `movups` de 4 floats contigus sans reconstruction, comme dans la version SSE SoA déjà faite en C++.

---

## Sources et outils utilisés

- Google Benchmark (mesures chiffrées)
- Visual Studio, outil Utilisation de l'UC (profiler) et fenêtre Code Machine (désassemblage)
- MASM (Microsoft Macro Assembler, intégré à MSVC)
- Assistance IA (Claude) pour la mise en page du rapport
