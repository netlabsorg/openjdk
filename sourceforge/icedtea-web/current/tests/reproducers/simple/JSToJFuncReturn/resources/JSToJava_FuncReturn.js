function doFuncReturnTests(){
    var urlArgs = document.URL.split("?");
    var applet = document.getElementById('jstojFuncReturnApplet');
    var method = urlArgs[1];

    eval('var value = applet.' + method + '()');

    var checked_string = typeof(value)+' ';
    if( method === '_JSObject'){
        checked_string = checked_string +value.key1;
    }else{
        checked_string = checked_string +value;
    }

    applet.printStringAndFinish(checked_string);
}

