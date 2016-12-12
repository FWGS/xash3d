#!/bin/sh
if [ "$TRAVIS_PULL_REQUEST" != "false" ]; then
 echo "Travis should not deploy from pull requests"
 exit 0
else
 SOURCE_NAME=$1
 shift
 mkdir xash3d-travis
 cp -a $* xash3d-travis/
 cd xash3d-travis
 git init
 git config user.name FWGS-deployer
 git config user.email FWGS-deployer@users.noreply.github.com
 git remote add travis-deploy-public https://FWGS-deployer:${GH_TOKEN}@github.com/FWGS/xash3d-deploy.git
 echo \# $TRAVIS_BRANCH branch autobuilds from $SOURCE_NAME >> README.md
 echo >> README.md
 echo [code on github]\(https://github.com/FWGS/xash3d/tree/$TRAVIS_COMMIT\) >> README.md
 echo >> README.md
 echo [changelog for this build]\(https://github.com/FWGS/xash3d/commits/$TRAVIS_COMMIT\) >> README.md
 echo >> README.md
 for arg in $*; do
  echo \* [$arg]\(https://github.com/FWGS/xash3d-deploy/blob/$SOURCE_NAME-$TRAVIS_BRANCH/$arg\?raw\=true\) >> README.md
  echo >> README.md
 done
 git add .
 git commit -m "Laterst travis deploy $TRAVIS_COMMIT"
 git checkout -b $SOURCE_NAME-$TRAVIS_BRANCH
 git push -q --force travis-deploy-public $SOURCE_NAME-$TRAVIS_BRANCH
 git checkout -b $SOURCE_NAME-laterst
 git push -q --force travis-deploy-public $SOURCE_NAME-laterst
fi
exit 0