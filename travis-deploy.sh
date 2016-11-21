#!/bin/sh
if [ "$TRAVIS_PULL_REQUEST" != "false" ]; then
 echo "Travis should not deploy from pull requests"
 exit 0
else
 mkdir xash3d-travis
 cp -a $* xash3d-travis/
 cd xash3d-travis
 git init
 git config user.name mittorn
 git config user.email mittorn@sibmail.com
 git remote add travis-deploy-public https://mittorn:${GH_TOKEN}@github.com/mittorn/xash3d-travis.git
 git add .
 git commit -m "Laterst travis deploy"
 git push --force travis-deploy-public master
fi
