#!/bin/sh
if [ "$TRAVIS_PULL_REQUEST" != "false" ]; then
 echo "Travis should not deploy from pull requests"
 exit 0
else
 mkdir xash3d-travis
 cp -a $* xash3d-travis/
 cd xash3d-travis
 git init
 git config user.name FWGS-deployer
 git config user.email FWGS-deployer@users.noreply.github.com
 git remote add travis-deploy-public https://FWGS-deployer:${GH_TOKEN}@github.com/FWGS/xash3d-deploy.git
 git add .
 git commit -m "Laterst travis deploy $TRAVIS_COMMIT"
 git checkout -b travis-$TRAVIS_BRANCH
 git push --force travis-deploy-public travis-$TRAVIS_BRANCH
fi
exit 0