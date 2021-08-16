# **Documentation**

## R5Reloaded

Un projet Visual Studio basé sur Detours pour hook le moteur graphique.

TO-DO: Ajouter des informations additionnelles ici.

---
## Installation

Rendez-vous dans [Installation.](installation/install)

---
## Mon jeu n'arrête pas de crash !

Voir : [FAQ #Y'a t-il des bugs ?](faq/faq#are-there-bugs)

---
## Héberger un serveur

Voir : [Héberger un Serveur](servers/hosting)

---
## FAQ

Rendez-vous dans [FAQ.](faq/faq)

---

## Détail Important :
*  Ceci n'est pas un logiciel de triche ou de hack. N'essayez pas de l'utiliser sur la dernière version du jeu.
*   Quand vous utilisez R5Net vous communiquez votre adresse IP à notre base de données qui sera stockée jusqu'à la fin de votre hébergement de serveur. Ceci est requis pour pouvoir se connecter aux serveurs des autres joueurs.

## R5Net DISCLAIMER

Quand vous utilisez R5Net vous communiquez votre adresse IP à notre base de données qui sera stockée jusqu'à la fin de votre hébergement de serveur.

Ceci est requis pour pouvoir se connecter aux serveurs des autres joueurs.

Il y a une Checkbox qui est désactivée par défaut qui vous autorise à héberger vers le Navigateur de Serveurs.

Si vous la cochez et ne mettez aucun mot de passe votre serveur sera visible dans le Navigateur de Serveurs.

Mais si vous mettez un mot de passe vous obtiendrez un token comme réponse, un mot de passe et votre serveur ne sera pas visible dans le Navigateur de Serveurs.

Les joueurs peuvent rejoindre les serveurs privés avec le bouton "Private Servers".

Si vous faites ça vous communiquerez quand même votre adresse IP à notre base de données qui sera stockée jusqu'à la fin de votre hébergement de serveur pour permettre aux joueurs de se connecter.

Vous pouvez aussi, comme moyen alternatif, héberger le serveur sans cocher la boite "Navigateur de Serveurs" et envoyer l'IP et le port aux personnes que vous souhaitez inviter.

TL;DR Si vous cochez la boite server browser dans la catégorie "Host Server", votre IP sera enregistrée jusqu'à ce que vous fermez votre serveur.

## Informations Additionnelles

(EN) The game will pop a debug terminal, that will forward your commands directly to the in-game source console backend. SQVM prints are also enabled, and the hooks will attempt to load game vscripts out of `platform\` relative from the game executable before falling back to the SearchPath's defined in GameInfo.txt or the in-memory VPK structures.
