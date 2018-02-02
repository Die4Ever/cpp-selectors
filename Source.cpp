#include <iostream>
#include <string>
#include <vector>
#include <string.h>
#include <memory>
#include <functional>
#include <algorithm>
#include <bitset>
using namespace std;
#ifdef WIN32
#include <Windows.h>
#endif

class Obj;
class ObjList
{
	friend class Obj;
	vector<shared_ptr<Obj>> items;

	void erase(shared_ptr<Obj> obj);
public:
	ObjList& operator+=(const ObjList &r);
	ObjList& operator+=(shared_ptr<Obj> p);
	ObjList AddChild(shared_ptr<Obj> obj);
	ObjList ObjList::MoveTo(ObjList &target);
	string description();
	size_t size();
	shared_ptr<Obj> operator[](size_t i);
	//ObjList operator[](size_t i);
	ObjList s(string q);
	ObjList();
	ObjList(shared_ptr<Obj> obj);

	//ObjList& on(string event_name, std::function<ObjList (ObjList,Obj*)> func);
	ObjList& on(string event_name, std::function<ObjList (ObjList,ObjList)> func);
	ObjList& off(string event_name);
	ObjList trigger(string event_name, ObjList arg);
	ObjList ForEachP(std::function<ObjList (ObjList,Obj*)> func, ObjList arg);
	ObjList ForEach(std::function<ObjList (ObjList,ObjList)> func, ObjList arg);
	ObjList& attr(string key, string val);
	string attr(string key);

	ObjList& AddClass(string classname);
	ObjList& RemoveClass(string classname);
	bool HasClass(string classname);
	bool AllHaveClass(string classname);
	ObjList& Remove();
	ObjList Remove(string q);

	ObjList operator()(string q);
	ObjList& operator()();
};

template<class t>
shared_ptr<t> Make(string classes, string newid);

template<class t>
auto BinarySearchPair(t &v, string key) -> decltype(v.end())
{
	if(v.size()==0) return v.end();
	auto i = lower_bound(v.begin(),v.end(), make_pair(key,decltype(v[0].second)()), [](const pair<string,decltype(v[0].second)> &a, const pair<string,decltype(v[0].second)> &b){
		return a.first<b.first;
	});
	if(i==v.end() || i->first!=key) return v.end();
	return i;
}

template<class t>
void SortPairs(t &v)
{
	if(v.size()==0) return;
	sort(v.begin(),v.end(), [](const pair<string,decltype(v[0].second)> &a, const pair<string,decltype(v[0].second)> &b){
		return a.first<b.first;
	});
}

template<class t>
void SortPairsStable(t &v)
{
	if(v.size()==0) return;
	stable_sort(v.begin(),v.end(), [](const pair<string,decltype(v[0].second)> &a, const pair<string,decltype(v[0].second)> &b){
		return a.first<b.first;
	});
}

template<class t>
int DeletePairs(t &v, string key)
{
	int count=0;
	auto a = BinarySearchPair(v, key);
	auto first=a;
	for(;a!=v.end() && a->first==key;++a) {
		count++;
	}
	if(first!=v.end()) {
		v.erase(first, a);//is it faster to do swaps, pop_backs, and then a sort?
	}
	return count;
}

class Obj
{
	friend class ObjList;
	template<class t>
	friend shared_ptr<t> Make(string, string);

	ObjList children;
	weak_ptr<Obj> wthis;
	weak_ptr<Obj> wparent;
	vector<string> classnames;
	vector<pair<string,string>> attrs;
	string id;
	vector<pair< string,std::function<ObjList (ObjList,ObjList)> >> event_listeners;
	bitset<128> childrenhash;

	bool HasClass(string classname) {
		return std::binary_search(classnames.begin(),classnames.end(), classname);
	}

	bool match_rule(string &r)
	{
		if(r=="*") return true;
		if(r[0]=='#' && id==r.c_str()+1) return true;
		if(r[0]=='.') {
			return std::binary_search(classnames.begin(),classnames.end(), r.substr(1));
		}
		if(r[0]=='[') {
			const char *s=r.c_str()+1;
			const char *e=strchr(s,'=');
			if(e) {
				string key(s,e-s);
				string val(e+1);
				val=val.substr(0,val.length()-1);
				if(attr(key)==val) return true;
			}
		}
		return false;
	}

	bool match(vector<string> &rules)
	{
		if(rules.size()==0) return false;
		size_t hits=0;
		for(auto &s : rules) {
			if(match_rule(s)) hits++;
			else return false;
		}
		if(hits==rules.size()) return true;
		return false;
	}

	int Hash(string v)
	{
		int h=0;
		int m=1;
		for(const char *s=v.c_str();*s;s++) {
			h=(*s)*m;
			m*=117;
		}
		return h;
	}

	void ChildChanged(shared_ptr<Obj> obj)
	{
		for(auto &s : obj->classnames) {
			childrenhash.set(Hash("."+s)%childrenhash.size());
		}
		childrenhash.set(Hash("#"+obj->id)%childrenhash.size());
		shared_ptr<Obj> sparent=wparent.lock();
		if(sparent) {
			sparent->ChildChanged(obj);
		}
	}

	bool CheckHash(vector<string> &rule)
	{
		for(auto &s : rule) {
			if(s[0]!='#' && s[0]!='.') continue;
			if(childrenhash[Hash(s)%childrenhash.size()]==false) return false;
		}
		return true;
	}

	void _find(vector<vector<string>> &rules, size_t r, ObjList &ret)
	{
		//ObjList ret;
		bool descend=true;
		if(rules[r][0]==">") {
			descend=false;
			r++;
		}
		if(CheckHash(rules[r])==false) {
			descend=false;
		}
		if(match(rules[r])) {
			//r++;
			if(rules.size()<=r+1) {
				auto pthis=wthis.lock();
				ret+=pthis;
			} else {
				for(size_t c=0;c<children.size();c++) {
					/*ret += */children.items[c]->_find(rules, r+1, ret);
				}
			}
			if(rules[r][0][0]=='#')
				return;// ret;
		}
		if(descend && rules.size()>r) {
			for(size_t c=0;c<children.size();c++) {
				/*ret += */children.items[c]->_find(rules, r, ret);
			}
		}
		return;// ret;
	}

	shared_ptr<Obj> AddChild(shared_ptr<Obj> obj)
	{
		children+=obj;
		if(wthis.expired()) {
			cerr << "\nyou're not supposed to AddChild in an Obj, put it in an ObjList first!\n";
			throw std::bad_weak_ptr("you're not supposed to AddChild in an Obj, put it in an ObjList first!");
		}
		obj->wparent=wthis;
		ChildChanged(obj);
		return obj;
	}

	void on(string event_name, std::function<ObjList (ObjList,ObjList)> func)
	{
		event_listeners.push_back( make_pair(event_name,func) );
		SortPairsStable(event_listeners);
	}

	int off(string event_name)
	{
		return DeletePairs(event_listeners, event_name);
	}

	ObjList trigger(string event_name, ObjList arg, ObjList lthis)
	{
		ObjList ret;
		auto a = BinarySearchPair(event_listeners, event_name);
		for(;a!=event_listeners.end() && a->first==event_name;++a) {
			ret += a->second(arg,lthis);
		}
		return ret;
	}

	void AddClass(string classes)
	{
		const char *s=classes.c_str();
		const char *e=s;
		while(*s) {
			if(*e==' '||*e==0) {
				string classname(s,e-s);
				if(std::binary_search(classnames.begin(),classnames.end(), classname)==false) {
					classnames.push_back(classname);
				}
				if(*e==0) break;
				s=e+1;
			}
			e++;
		}
		sort(classnames.begin(),classnames.end());
		shared_ptr<Obj> sparent=wparent.lock();
		if(sparent) {
			sparent->ChildChanged(wthis.lock());
		}
	}

	auto SearchClassnames(string classname) -> decltype(classnames.end())
	{
		auto &v= classnames;
		auto a = lower_bound(v.begin(),v.end(), classname);
		if(*a!=classname) return v.end();
		return a;
	}

	int RemoveClass(string classes)
	{
		int count=0;
		const char *s=classes.c_str();
		const char *e=s;
		while(*s) {
			if(*e==' '||*e==0) {
				string classname(s,e-s);
				auto a = SearchClassnames(classname);
				if(a!=classnames.end()) {
					auto first=a;
					for(;a!=classnames.end() && *a==classname; ++a) {
						count++;
					}
					classnames.erase(first,a);
				}
				if(*e==0) break;
				s=e+1;
			}
			e++;
		}
		sort(classnames.begin(),classnames.end());
		return count;
	}

	void RemoveChild(shared_ptr<Obj> obj)
	{
		children.erase(obj);
	}

	int Remove()
	{
		auto sthis = wthis.lock();
		if(!sthis) {
			cerr << "\nyou're not supposed to Remove an Obj, put it in an ObjList first!\n";
			throw bad_weak_ptr("you're not supposed to Remove an Obj, put it in an ObjList first!");
		}
		auto sparent=wparent.lock();
		if(sparent) {
			sparent->RemoveChild(sthis);
		}
		int refs=(int)sthis.use_count()-1;
		return refs;
	}

protected:
	Obj(string classes, string newid) : id(newid)
	{
		AddClass(classes);
	}

public:
	Obj(const Obj &obj) : id(obj.id), classnames(obj.classnames), event_listeners(obj.event_listeners), attrs(obj.attrs)
	{
	}
	Obj(Obj &&obj) : id(std::move(obj.id)), classnames(std::move(obj.classnames)), event_listeners(std::move(obj.event_listeners)), attrs(std::move(obj.attrs))
	{
		if(obj.wparent.lock() || obj.wthis.lock()) {
			cerr << "you're not supposed to move an Obj directly, use ObjLists to manage Objs\n";
			throw std::exception("you're not supposed to move an Obj directly, use ObjLists to manage Objs");
		}
	}

	virtual ~Obj()
	{
	}

	ObjList find(string q)
	{
		if(wthis.expired()) {
			cerr << "\nyou're not supposed to find in an Obj, put it in an ObjList first!\n";
			throw bad_weak_ptr("you're not supposed to find in an Obj, put it in an ObjList first!");
		}
		vector<vector<string>> rules;
		rules.push_back(vector<string>());
		const char *s=q.c_str();
		const char *e=s+1;
		while(*s) {
			if(*e==0 || strchr(" .#[:>",*e)) {
				string new_rule(s,e-s);
				//cout << "new rule '"<<new_rule<<"'\n";
				rules.back().push_back(new_rule);
				if(*e==' ') {
					rules.push_back(vector<string>());
					e++;
				}
				s=e;
			}
			e++;
		}
		if(rules.back().size()==0)
			return ObjList();
		/*cout << q << "\n";
		for(auto &rs : rules) {
		cout << "\t";
		for(auto &r : rs) {
		cout << "'"<<r<<"'";
		}
		cout << "\n";
		}*/
		ObjList ret;
		for(size_t c=0;c<children.size();c++) {
			//ret += children.items[c]->_find(rules, 0);
			children.items[c]->_find(rules, 0, ret);
		}
		return ret;
		//return _find(rules, 0);
	}

	template<class t>
	bool IsType()
	{
		if(typeid(*this)==typeid(t))
			return true;
		return false;
	}

	string description(int recurse=0)
	{
		return "#"+id;
		string ret;
		auto sparent=wparent.lock();
		if(sparent && recurse<3) ret += sparent->description(recurse+1)+" ";
		ret+="#"+id;
		if(recurse==0) {
			for(auto &c : classnames) {
				ret+="."+c;
			}
		}
		return ret;
	}

	virtual string attr(string key)
	{
		auto a = BinarySearchPair(attrs, key);
		if(a!=attrs.end()) return a->second;
		return string();
	}

	virtual void attr(string key, string val)
	{
		auto a = BinarySearchPair(attrs, key);
		if(a!=attrs.end()) {
			a->second=val;
			return;
		}
		attrs.push_back( make_pair(key,val) );
		SortPairs(attrs);
	}
};

ObjList& ObjList::operator+=(const ObjList &r)
{
	items.insert(items.end(), r.items.begin(), r.items.end());
	return *this;
}
ObjList& ObjList::operator+=(shared_ptr<Obj> p)
{
	if(p) {
		items.push_back(p);
	}
	return *this;
}

void ObjList::erase(shared_ptr<Obj> obj)
{
	for(size_t i=0;i<items.size();i++) {
		if(items[i]==obj) {
			items.erase(items.begin()+i);
			return;
		}
	}
}

ObjList ObjList::AddChild(shared_ptr<Obj> obj)
{
	ObjList ret;
	for(auto &i : items) {
		ret += i->AddChild(obj);
		i->trigger("add-child", ObjList(obj), ObjList(i));
	}
	return ret;
}
ObjList& ObjList::Remove()
{
	for(size_t i=0;i<items.size();i++) {
		auto &obj=items[i];
		obj->Remove();
		items.erase(items.begin()+i);
	}
	return *this;
}
ObjList ObjList::Remove(string q)
{
	ObjList ret=s(q);
	for(auto &i : ret.items) {
		erase(i);
	}
	ret.Remove();
	return ret;
}
ObjList ObjList::MoveTo(ObjList &target)
{
	if(size()!=1) {
		cerr << "you can only move 1 Obj at once! you have "<<size()<<" selected!\n";
		throw std::exception("you can only move 1 Obj at once!");
	}
	if(target.size()!=1) {
		cerr << "you can only move into 1 parent! you have "<<target.size()<<" parents selected!\n";
		throw std::exception("you can only move into 1 parent!");
	}
	items[0]->Remove();
	target.AddChild(items[0]);
	return *this;
}
ObjList& ObjList::AddClass(string classname)
{
	for(auto &i : items) {
		i->AddClass(classname);
	}
	return *this;
}
ObjList& ObjList::RemoveClass(string classname)
{
	for(auto &i : items) {
		i->RemoveClass(classname);
	}
	return *this;
}
bool ObjList::HasClass(string classname)
{
	for(auto &i : items) {
		if(i->HasClass(classname)) return true;
	}
	return false;
}
bool ObjList::AllHaveClass(string classname)
{
	for(auto &i : items) {
		if(i->HasClass(classname)==false) return false;
	}
	return true;
}
ObjList& ObjList::off(string event_name)
{
	for(auto &i : items) {
		i->off(event_name);
	}
	return *this;
}
string ObjList::description()
{
	string ret;
	//for(auto &i : items) {
	for(size_t a=0;a<size();a++) {
		auto &i = items[a];
		ret += i->description();
		if(a<size()-1) ret += ", ";
	}
	return ret;
}
size_t ObjList::size()
{
	return items.size();
}
shared_ptr<Obj> ObjList::operator[](size_t i)
{
	/*ObjList ret;
	ret.items.push_back(items[i]);
	return ret;*/
	return items[i];
}
ObjList ObjList::s(string q)
{
	ObjList ret;
	for(auto &i : items) {
		ret += i->find(q);
	}
	return ret;
}
ObjList ObjList::operator()(string q)
{
	return s(q);
}

ObjList& ObjList::operator()()
{
	return *this;
}

ObjList::ObjList()
{
}
ObjList::ObjList(shared_ptr<Obj> obj)
{
	items.push_back(obj);
	//items.back()->wthis=items.back();
}

/*ObjList& ObjList::on(string event_name, std::function<ObjList (ObjList, Obj*)> func)
{
	for(auto &i : items) {
		//i->event_listeners.push_back( make_pair(event_name, func) );
		i->on(event_name, func);
	}
	return *this;
}*/
ObjList& ObjList::on(string event_name, std::function<ObjList (ObjList, ObjList)> func)
{
	for(auto &i : items) {
		//i->event_listeners.push_back( make_pair(event_name, func) );
		i->on(event_name, func);
	}
	return *this;
}
ObjList ObjList::trigger(string event_name, ObjList arg)
{
	ObjList ret;
	for(auto &i : items) {
		ret+=i->trigger(event_name, arg, ObjList(i));
	}
	return ret;
}
ObjList ObjList::ForEachP(std::function<ObjList (ObjList,Obj*)> func, ObjList arg)
{
	ObjList ret;
	for(auto &i : items) {
		ret+=func(arg, i.get());
	}
	return ret;
}
ObjList ObjList::ForEach(std::function<ObjList (ObjList,ObjList)> func, ObjList arg)
{
	ObjList ret;
	for(auto &i : items) {
		ret+=func(arg, ObjList(i));
	}
	return ret;
}
ObjList& ObjList::attr(string key, string val)
{
	for(auto &i : items) {
		i->attr(key,val);
	}
	return *this;
}

string ObjList::attr(string key)
{
	if(items.size()) return items[0]->attr(key);
	return string();
}

template<class t>
shared_ptr<t> Make(string classes, string newid)
{
	auto p = make_shared<t>( t(classes,newid) );
	p->wthis=p;
	return p;
}

class Character : public Obj
{
	int hp;
	int str;
public:
	Character(string classes, string newid) : Obj(classes, newid)
	{
		hp=15;
		str=10;
	}

	static shared_ptr<Character> FromObj(shared_ptr<Obj> obj)
	{
		if( obj->IsType<Character>()==false) throw std::bad_typeid("obj is not a Character");
		return std::dynamic_pointer_cast<Character>(obj);
	}

	virtual string attr(string key)
	{
		if(key=="hp") return string("15");
		if(key=="str") return string("10");
		return Obj::attr(key);
	}

	virtual void attr(string key, string val)
	{
		Obj::attr(key,val);
	}
};

ObjList w(Make<Obj>("root","root"));

int test()
{
	/*
	To create an Obj you just use the Make function and specify what type of object you want, the classnames, and the id, like this -
		auto oldtree = Make<Obj>("tree", "old-as-fuck-tree");

	But usually you want to put it in an ObjList. There is a default ObjList for the root of everything
	so you would just use AddChild on the ObjList root or world like this -
		w.AddChild( Make<Obj>("area city mountains", "payon") );
	OR
		w("#prontera").AddChild( Make<Obj>("shop building","item-shop") );

	When creating an Obj the first string is a space delimited list of classes, the second string is the ID
	To select items, you use # to mean ID . to mean class and [attr=val] to look for an attribute, see below for examples.

	You will probably want to make a class for different things like Chatacters, Shops, Areas; I have an example Character class which inherits from the Obj class

	You can add a character like this
		w("#prontera #item-shop").AddChild( MakeObj<Character>("character player human badass-mofo", "samuel-l-jackson") );
	To use character specific features like accessing their hp or str directly(which is faster for you and the cpu than using the attr() functions to do it)
		auto sam_jackson = Character::FromObj(w("#prontera #samuel-l-jackson")[0]);
		sam_jackson->hp=10000;
		sam_jackson->str=10000;

	Also take a look at the examples for events
	Adding an event handler -
		w(".character").on("smacked", [](ObjList arg, Obj *pthis) -> ObjList {
			cout << "Ow why did you smack me, "<<arg.description()<<"?";
		});

	Removing event handlers
		w("#samuel-l-jackson").off("smacked");

	Triggering the event
		w(".character").trigger("smacked", ObjList());//this is with no arguments passed to it, an empty set
		w(".character").trigger("smacked", world.s("#samuel-l-jackson"));//now all the characters get smacked by sam jackson
		//yea, I need a way to select everything that doesn't match an ID or class...

	With events, keep in mind that adding multiple event handlers with the same name will mean they all get called, so often times you'll want to do like this -
		w(".character").off("smacked").on("smacked", [](ObjList arg, Obj *pthis) -> ObjList {
			cout << "Ow why did you smack me, "<<arg.description()<<"?";
		});

	This way it clears out all the old event handlers on the smacked event, and now there's only the new one you just added.

	Adding a class
		w("#samuel-l-jackson").AddClass("actor rich famous good-speaker");

	Removing a class, and also chaining functions
		w.RemoveClass("flat").AddClass("round");//we've made a discovery!

	Removing items
		w(".character").Remove();//removes all characters from the world
		world.Remove(".character");//same thing


		if(w(".character").HasClass("human")) cout << "at least 1 character is a human!\n";//this will return true since we have at least 1 human character

		if(w(".character").AllHaveClass("human")) cout << "every character is a human!\n";//this will return true only if all characters have the human class, so if we have an orc character this will return false
		else cout << "not every character is a human!\n";

	Look at the example code I have below, some of it is a bit more advanced, like the ForEach function, or the chaining of the functions
	*/
	w.AddChild( Make<Obj>("area city mountains", "payon") );
	w.AddChild( Make<Obj>("area city forest", "prontera") );
	w.AddChild( Make<Obj>("area city forest", "alberta") );

	cout << w(".city").description()<<"\n";
	w(".city").AddChild( Make<Obj>("shop building","weapon-shop") ).AddChild( Make<Obj>("item weapon","rusty-sword") );
	w(".shop").AddChild( Make<Obj>("item weapon", "badass-axe") );
	cout << w(".forest .shop .item").description() << "\n";

	w("#payon").AddChild( Make<Character>("character player human","player") );
	w("#payon").AddChild( Make<Character>("character","dude") ).attr("name","justin");

	auto player = Character::FromObj(w("#payon #player")[0]);
	player->attr("foo","bar");
	player->attr("name","grag");
	cout << player->attr("foo")<<"\n";

	cout << "name "<<w(".character[name=grag]").description()<<"\n";
	cout << "\n\n";
	w("#payon #player").on("attack", [](ObjList arg, ObjList lthis) -> ObjList {
		cout <<lthis.description() <<" attacking "<<arg.description()<<"\n";
		return ObjList();
	}).on("run-to", [](ObjList arg, ObjList lthis) -> ObjList {
		cout <<lthis.description() <<" running to "<<arg[0]->description()<<"\n";
		return ObjList();
	}).on("run-to", [](ObjList arg, ObjList lthis) -> ObjList {
		cout <<lthis.description() <<" running even faster to "<<arg[0]->description()<<"\n";
		return ObjList();
	});;

	w("#payon #player").trigger("run-to", w(".shop"));
	w("#payon #player").trigger("attack", w(".character[name=justin]"));
	w("#payon #player").trigger("fjshdkfsl", w(".character[name=justin]"));

	cout << "\nRemove() test\n";
	cout << "before Remove -\n"<<w(".character").description()<<"\n------------------\n";
	cout << w.Remove("#asshole").description() << "\n";
	cout << "after Remove -\n"<<w(".character").description()<<"\n------------------\n";
	w("#player").RemoveClass("character");
	cout << "after Remove .character -\n"<<w(".character").Remove().description() << "\n";
	cout << w("#player").description()<<"\n";

	w(".city").ForEachP( [](ObjList arg, Obj *pthis) -> ObjList {
		cout << "from ForEach() "<<pthis->description() << "\n";
		return ObjList();
	}, ObjList());

	//w(".city .weapon-shop[name='bla'][x=100][y=100]:active .item.weapon[cost<1000]");
	//still need [attr>val] and [attr<val], and also need more dynamic things like selectors for near-player
	return 0;
}

int main()
{
	test();
	w=ObjList(Make<Obj>("root","root"));

	auto s = GetTickCount64();
	w.AddChild(Make<Obj>("region","north"));
	w.AddChild(Make<Obj>("region","south"));
	w.AddChild(Make<Obj>("region","east"));
	w.AddChild(Make<Obj>("region","west"));

	w("#north").AddChild(Make<Obj>("area city","prontera"));
	w("#east").AddChild(Make<Obj>("area city payon","payon"));
	w("#south").AddChild(Make<Obj>("area city","alberta"));
	w("#north").AddChild(Make<Obj>("area city","aldabaran"));
	w("#west").AddChild(Make<Obj>("area city","geffen"));
	w("#west").AddChild(Make<Obj>("area city","morroco"));
	w(".city").on("add-child", [](ObjList arg, ObjList lthis) -> ObjList {
		return ObjList();
	});

	for(int i=0;i<10;i++)
		w(".region").AddChild(Make<Obj>("area field", "field"));

	for(int i=0;i<10;i++)
		w(".city").AddChild(Make<Obj>("building house","house"));
	w(".city").AddChild(Make<Obj>("building shop weapon-shop", "weapon-shop"));
	w(".city").AddChild(Make<Obj>("building shop armor-shop", "armor-shop"));
	w(".city").AddChild(Make<Obj>("building shop item-shop", "item-shop"));

	for(int i=0;i<5000;i++)
		w("> .region > .city").AddChild(Make<Character>("character human npc","npc"));
	for(int i=0;i<4;i++)
		w(".city .house").AddChild(Make<Character>("character human npc","npc"));
	for(int i=0;i<10;i++)
		w(".city .shop").AddChild(Make<Character>("character human npc","npc"));

	w("#payon").AddChild(Make<Character>("character human player","player"));
	w("#player").MoveTo(w("#prontera"));
	auto e = GetTickCount64();
	cout << "Took "<<(e-s)<<"ms\n";

	cout << "--------\n";
	cout << "player in #payon? "<<w("#payon #player").description()<<"\n";
	cout << "player in #prontera? "<<w("#prontera #player").description()<<"\n";
	w("#prontera #player").MoveTo(w("#payon"));
	cout << "--------\n";
	cout << "player in #payon? "<<w("#payon #player").description()<<"\n";
	cout << "player in #prontera? "<<w("#prontera #player").description()<<"\n";
	cout << "--------\n";

	cout << w("> .region > .payon").description()<<"\n";
	w("#player").AddChild(Make<Character>("fairy player","player"));
	cout << w(".player").description()<<"\n";
}
