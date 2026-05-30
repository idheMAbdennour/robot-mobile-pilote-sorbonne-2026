# Guide Pratique et Théorique : Odométrie Relative et Asservissement

Ce guide détaille la méthode complète pour asservir votre robot différentiel à 50Hz. Il fusionne la théorie de **l'odométrie relative** (prédiction par les roues) et la **correction par capteurs inductifs** (mesure), tout en prenant en compte vos contraintes électroniques et matérielles.

## 1. Objectif du Modèle (Odométrie Relative)

L'objectif n'est **pas** de localiser le robot dans le plan absolu $(x, y, \theta)$, ni de connaître sa position longitudinale sur le fil ($s$). Le problème est réduit à une odométrie relative locale.

L'état utile du robot se résume à deux grandeurs :

- $y$ : l'écart latéral par rapport au fil (en mètres).
- $\alpha$ : l'erreur angulaire entre le robot et le fil (en radians).

L'objectif de l'asservissement est de maintenir : $y \approx 0$ et $\alpha \approx 0$.

---

## 2. Étape 1 : Calibrage de la Mesure Inductive ($y_{mes}$ et $\alpha_{mes}$)

Avant de faire des mathématiques, il faut extraire des mesures fiables du fil.

### A. Dimensionnement Électronique (Hardware)

L'acquisition passe par une chaîne qu'il faut régler avec précision :

1. **Signal Brut & Filtre** : Le champ magnétique 50kHz est capté puis filtré par un RLC passe-bande.
2. **Amplification (AOP)** : Le signal est très faible. On l'amplifie avec un AOP (alim 0-12V, masse virtuelle $V_{ref}$ = 6V).
3. **Adaptation ADC** : On utilise un pont diviseur pour atténuer et recentrer le signal (12V) autour de 1.5V (3V crête-à-crête) pour le protéger et l'adapter à l'ADC du microcontrôleur.

> **TEST & RÉGLAGE** : Placez le robot à la distance maximale utile (ex: 15cm). Réglez le gain de l'AOP pour que l'amplitude atteigne **exactement** 12V crête-à-crête (donnant 3V càc sur l'ADC). Cela maximise votre résolution.

### B. Linéarité et Calcul des Mesures

En divisant les amplitudes (ex: ADC1/ADC3), on obtient un rapport $r$.

- **Bonne nouvelle** : Ces rapports sont **linéaires** pour des distances $< 15$ cm et des angles $< \pm 15^\circ$.
- Vous pouvez donc déterminer de simples coefficients multiplicateurs par calibration physique (mesures à 0, 5, 10, 15cm) pour convertir le rapport $r$ en $y_{mes}$ (en mètres) et $\alpha_{mes}$ (en radians).

### C. Détection de Validité

Définissez un flag `fil_valide`. Si l'amplitude du signal est trop faible (ou saturée), `fil_valide = 0`. Sinon, `fil_valide = 1`.

---

## 3. Étape 2 : Prédiction par les Encodeurs (Odométrie)

Les roues peuvent glisser et les PWM ne sont pas linéaires. Cependant, sur un court instant ($\Delta t = 20ms$ à 50Hz), l'odométrie est très utile pour prédire le mouvement.

On définit $e$ comme l'entraxe entre les deux roues. À chaque tick, on lit les déplacements des encodeurs ($\Delta d_g$ et $\Delta d_d$) en mètres :

$$ \Delta d = \frac{\Delta d_d + \Delta d_g}{2} $$
$$ \Delta \theta = \frac{\Delta d_d - \Delta d_g}{e} $$

On met ensuite à jour l'état prédit ($\hat{y}_{k+1}$ et $\hat{\alpha}_{k+1}$) :

$$ \hat{y}_{k+1} = y_k + \Delta d \cdot \sin\left(\alpha_k + \frac{\Delta \theta}{2}\right) $$
$$ \hat{\alpha}_{k+1} = \alpha_k + \Delta \theta $$

*(Note : Pour de très petits angles, on peut approximer le sinus : $\sin(\alpha) \approx \alpha$)*.

---

## 4. Étape 3 : Correction par les Capteurs (Fusion de Données)

L'odométrie seule dérive dans le temps. Le capteur inductif sert à recaler cette prédiction.
C'est ici qu'intervient la fusion :

```c
if (fil_valide == 1) {
    // K_corr_y et K_corr_alpha sont des gains entre 0 et 1.
    // 0 = On ne fait confiance qu'aux roues (odométrie).
    // 1 = On ne fait confiance qu'aux capteurs (mesure pure).
    // Une valeur intermédiaire (ex: 0.5) filtre le bruit du capteur.
    y_k1   = y_pred   + K_corr_y   * (y_mes   - y_pred);
    alpha_k1 = alpha_pred + K_corr_alpha * (alpha_mes - alpha_pred);
} else {
    // Si on perd le fil, on navigue à l'aveugle via l'odométrie
    // en espérant le retrouver rapidement.
    y_k1   = y_pred;
    alpha_k1 = alpha_pred;
}
```

L'état de votre robot ($y$ et $\alpha$) est maintenant estimé de manière robuste et propre !

---

## 5. Étape 4 : La Loi de Commande Moteur

Maintenant que vous connaissez $y$ et $\alpha$, la commande vise à les ramener à zéro.
On calcule une commande de vitesse angulaire $\omega_{cmd}$ :

$$ \omega_{cmd} = - K_y \cdot y - K_\alpha \cdot \alpha $$

- $K_y$ : Gain pour corriger l'écart. S'il est trop fort, le robot oscille autour du fil.
- $K_\alpha$ : Gain pour corriger l'angle. Il "amortit" la trajectoire.

**Conversion en vitesses de roues :**
La centrale vous impose une vitesse linéaire $v_{cmd}$ (que vous lissez via une rampe). On la combine avec la commande angulaire :

$$ v_g = v_{cmd} - \frac{e}{2} \cdot \omega_{cmd} $$
$$ v_d = v_{cmd} + \frac{e}{2} \cdot \omega_{cmd} $$

### Saturation et PWM

Ces vitesses $v_g$ et $v_d$ sont théoriques (en m/s). Vous devez ensuite les convertir en consignes PWM.
C'est ici que l'asymétrie des moteurs intervient. Une boucle de régulation de vitesse par roue (PI) est recommandée pour s'assurer que la vitesse réelle de la roue corresponde bien à la consigne demandée, effaçant ainsi la non-linéarité des PWM.

**Sécurités finales avant d'envoyer aux moteurs :**

```c
// Pas de marche arrière
if (PWM_g < 0) PWM_g = 0;
if (PWM_d < 0) PWM_d = 0;

// Saturation max
if (PWM_g > PWM_MAX) PWM_g = PWM_MAX;
if (PWM_d > PWM_MAX) PWM_d = PWM_MAX;
```

---

## Résumé de l'algorithme complet à 50Hz

1. **Mesure Roues** : Lire les encodeurs $\rightarrow$ Calculer $\Delta d$ et $\Delta \theta$.
2. **Prédiction Odométrique** : Calculer $\hat{y}$ et $\hat{\alpha}$.
3. **Mesure Capteurs** : Lire les ADC $\rightarrow$ Filtrer $\rightarrow$ Si valide, calculer $y_{mes}$ et $\alpha_{mes}$.
4. **Correction** : Mettre à jour l'état final $y$ et $\alpha$ (Fusion de données).
5. **Calcul Commande** : Générer $\omega_{cmd}$ via un contrôleur Proportionnel sur $y$ et $\alpha$.
6. **Mixage** : Calculer les consignes de vitesses cibles pour les roues gauche et droite.
7. **Régulation Roues** : (Optionnel mais recommandé) Asservir chaque roue individuellement pour atteindre ces consignes.
8. **Application** : Envoyer les PWM aux moteurs.
