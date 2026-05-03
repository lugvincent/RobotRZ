A([Flux audio]) --> B{Pilotage vocal ?}
    B -- Non --> Z([Fin])
    B -- Oui --> C{Mode utilisateur}
    C -- Conversation --> D{Internet OK ?}
    D -- Oui --> E[STT IA Cloud]:::external
    D -- Non --> F[Bascule auto Commande]:::process
    F --> G[TTS 'Réseau indispo']:::tts
    G --> H
    C -- Commande --> H

    subgraph Sphinx [Moteur Sphinx]
        H[Activer Sphinx] --> I[Lire contexte]
        I --> J[Adapter grammaire]
        J --> K[Reconnaissance]
    end

    K -- Échec --> L[Retour écoute]
    K -- Succès --> M[Texte + valeur numérique]

    M --> N[Recherche dans catalogue]
    N -- Trouvé --> O[ID_COMMANDE]
    N -- Non trouvé --> P{Conversation possible ?}
    P -- Oui --> D
    P -- Non --> Q[TTS 'Commande inconnue']:::tts
    Q --> Z

    O --> R{CPU > 90% ?}
    R -- Oui --> S[TTS 'Surchargé']:::tts --> Z
    R -- Non --> T{modeRZ}

    T -- ERREUR --> U[Forcer URGENCE]
    T -- ARRET --> V[TTS 'Système éteint']:::tts --> Z
    T -- VEILLE/FIXE/DÉPLACEMENT --> W{modePtge}
    T -- AUTONOME --> X[Validation locale<br>State‑Manager]

    W -- Vocal --> Y[Commande valide]
    W -- Vocal‑Laisse --> ZA{Mtr.speed ≠ 0?}
    ZA -- Non --> Y
    ZA -- Oui --> ZB{ID == STOP?}
    ZB -- Oui --> Y
    ZB -- Non --> ZC[TTS 'Laisse active, moteur bloqué']:::tts --> Z

    U --> Y
    X --> Y

    Y --> AA[Traitement catalogue]
    AA --> AB{Type traitement}

    AB -- SIMPLE --> AC[Construire VPIV]:::vpiv
    AC --> AD[CAT,VAR,PROP,INST,VAL]
    AD --> AE[Appliquer règle destinataire]

    AB -- COMPLEXE --> AF[Boucle sur ID_FILS]:::process
    AF --> AG[Chaque fils -> SIMPLE]
    AG --> AE

    AB -- LOCAL --> AH{CAT VPIV}
    AH -- A --> AI[Action Tasker locale<br>+ copie CAT:I vers SP]
    AH -- I --> AJ[Action SE interne<br>+ TTS si défini]
    AH -- Autre --> AK[Action locale sans SP]

    AI & AJ & AK --> AL[Exécution locale directe]:::process
    AL --> AM{Feedback vocal ?}

    AE --> AN{modeRZ == AUTONOME?}
    AN -- Oui --> AO[Validation State‑Manager] --> AP[Exécution]
    AN -- Non --> AQ[Envoi VPIV vers SP]:::vpiv
    AQ --> AR[FIFO termux_in]
    AR --> AS[parse_vpiv]
    AS --> AT[MQTT]

    subgraph Ack [Attente ACK SP]
        AU[Attente ACK/NACK] --> AV{Délai < seuil?}
        AV -- Oui --> AW{ACK?}
        AW -- Oui --> AX[Exécution]
        AW -- Non --> AY[TTS erreur SP]:::tts
        AV -- Non --> AZ[TTS timeout]:::tts
    end