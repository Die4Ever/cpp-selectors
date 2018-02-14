# cpp-selectors
JQuery/CSS style selectors in C++

examples:
w("#prontera #item-shop").AddChild( MakeObj<Character>("character player human badass-mofo", "samuel-l-jackson") );

w(".character").on("smacked", [](ObjList arg, Obj *pthis) -> ObjList {
			cout << "Ow why did you smack me, "<<arg.description()<<"?";
		});
  
w(".forest .shop .item")

w(".character[name=grag]")
