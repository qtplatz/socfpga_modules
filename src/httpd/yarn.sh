#!/usr/bin/bash

if ! command -v yarn &> /dev/null
then
	sudo npm install -g yarn
fi

cd html

[ ! -d "node_modules/angular-plotly" ]		&& yarn add angular-plotly
[ ! -d "node_modules/bootstrap" ]           && yarn add bootstrap
[ ! -d "node_modules/bootstrap-toggle" ]    && yarn add bootstrap-toggle
[ ! -d "node_modules/bootstrap-fileinput" ] && yarn add bootstrap-fileinput
[ ! -d "node_modules/d3" ]                  && yarn add d3
[ ! -d "node_modules/file-saver" ]			&& yarn add filesaver
[ ! -d "node_modules/font-awesome" ]        && yarn add font-awesome
[ ! -d "node_modules/jquery" ]              && yarn add jquery
[ ! -d "node_modules/jquery-ui" ]           && yarn add jquery-ui
[ ! -d "node_modules/jquery.terminal" ]     && yarn add jquery.terminal
[ ! -d "node_modules/jsgrid" ]              && yarn add js-grid
[ ! -d "node_modules/plotlyjs" ]            && yarn add plotly
[ ! -d "node_modules/sprintf" ]				&& yarn add sprintf