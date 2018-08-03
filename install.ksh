#!/bin/bash

PROJET=asterisk13
#Repertoire temporaire utiliser pour preparer les packages
TEMPDIR=/tmp

function svn_export
{
        svn export http://svn.ives.fr/svn-asterisk/tags/1.4.19m-videocaps ${PROJET}
}

#Preparation du fichier spec de packaging rpm

#Creation de l'environnement de packaging rpm
function create_rpm
{
    #Cree l'environnement de creation de package
    #Creation des macros rpmbuild
    rm -f ~/.rpmmacros
    touch ~/.rpmmacros
    echo "%_topdir" $HOME"/rpmbuild" >> ~/.rpmmacros
    echo "%_tmppath %{_topdir}/TMP" >> ~/.rpmmacros
    echo "%_signature gpg" >> ~/.rpmmacros
    echo "%_gpg_name IVeSkey" >> ~/.rpmmacros
    echo "%_gpg_path" $HOME"/rpmbuild/gnupg" >> ~/.rpmmacros
    echo "%vendor IVeS" >> ~/.rpmmacros
    #Import de la clef gpg IVeS
    mkdir -p $HOME/rpmbuild
    cd $HOME/rpmbuild
    svn export http://svn.ives.fr/svn-libs-dev/gnupg
    mkdir -p SOURCES
    mkdir -p SPECS
    mkdir -p BUILD
    mkdir -p SRPMS
    mkdir -p TMP
    mkdir -p RPMS
    mkdir -p RPMS/noarch
    mkdir -p RPMS/x86_64
    mkdir -p RPMS/i386
    mkdir -p RPMS/i686
    mkdir -p RPMS/i586
    #Recuperation de la description du package 
    cd -
    cp ${PROJET}.spec $HOME/rpmbuild/SPECS/${PROJET}.spec

    # copie de l'arbre de source dans le rep de build RPM
    # pour repasser en SVN, commenter les 3 lignes suivantes
    # et decommenter la ligne svn dans la cible %spec du fichier
    # asteriskv.spec.ives
    echo "Nettoyage et copie de l'arbre de source vers l'env de build RPM"
    make clean > /dev/null
    rm -rf $HOME/rpmbuild/SOURCES/${PROJET}
    cp -rp . $HOME/rpmbuild/SOURCES/${PROJET}

     if [[ -z $1 || $1 -ne nosign ]]
    then
             rpmbuild -bb --sign $HOME/rpmbuild/SPECS/${PROJET}.spec
             ret=$?
    else
             rpmbuild -bb $HOME/rpmbuild/SPECS/${PROJET}.spec
             ret=$?
    fi

    if [ $ret -ne 0 ]
    then
        exit 20;
    fi

    
    echo "************************* fin du rpmbuild ****************************"
    clean
}

function clean
{
  	# On efface les liens ainsi que le package precedemment créé
  	echo Effacement des fichiers et liens gnupg rpmbuild ${PROJET}.rpm ${TEMPDIR}/${PROJET}
  	rm -rf $HOME/rpmbuild/SPECS/${PROJET}.spec $HOME/rpmbuild/gnupg $HOME/rpmbuild/SOURCES/${PROJET}
}

case $1 in
  	"clean")
  		echo "Nettoyage des liens et du package crees par la cible dev"
  		clean ;;
  	"rpm")
  		echo "Creation du rpm"
  		create_rpm $2;;
  	*)
  		echo "usage: install.ksh [options]" 
  		echo "options :"
  		echo "  rpm		Generation d'un package rpm"
  		echo "  clean		Nettoie tous les fichiers cree par le present script, liens, tar.gz et rpm";;
esac
