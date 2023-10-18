pipeline ASTERISK 20

    GitHub => Modif interne à Asterisk + Modules internes
    GitLab => Composant NodeJS + packaging d'asterisk (patch, doc, specs) + modules externes

    Configs et modules externes Asterisk chacun dans un RPM séparés
        asterisk-X.Y.Z-R.ives.el8.x64_86.rpm            --> Asterisk + modules internes + modules externes
        asterisk-config-X.Y.Z-R.ives.el8.x64_86.rpm.    --> Config Asterisk + Config Modules Internes + Config Modules Externes
            Requires: Asterisk >= 21

    - Build Asterisk (MAJ le menuselect)
    - Build des modules externes (requires asterisk headers/devel)
    - Génération RPM

>    - (Tests unitaire / fonctonnel ? Tests en charge)
>    - Génération image Docker privée



Tâches :
- (B) Création du dépôt GitLab asterisk
- (B) Branches 20 + 21
- (JP) Upload Branch
- (JP) Donner nom modules externes
- (B) Intégrer les 2 modules externes dans une génération (= build binaires asterisk)
