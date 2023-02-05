/*
< Tool for managing bibTeX file as reference.
<
< Relevant Qt tutorials:
<   https://doc.qt.io/qt-6/qtsql-cachedtable-example.html
<   https://doc.qt.io/qt-6/qtwidgets-mainwindows-menus-example.html
<   https://doc.qt.io/qt-6/qtwidgets-dialogs-tabdialog-example.html
<
< Btparse related manuals:
<   https://www.dragonflybsd.org/cgi/web-man?command=bt_traversal
*/

#include <iostream>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableView>
#include <QtWidgets/QVBoxLayout>
#include <QtCore/QString>
#include <btparse.h>


/*
String constants for column names.
*/
const char * const Column_Names[]={"Name","Publication/Journal","Year","Entry type","Entry key"};

/*
Types of entries supported and field mappings to columns.
Each supported entry has it's own fields.
*/
enum Supported_BibTex_Entries{
	TEXBIB_ENTRY_ARTICLE=0,
	TEXBIB_ENTRY_BOOK=1,
	TEXBIB_ENTRY_GENERIC_NO_SUPPORT
};
const char *const Supported_BibTex_Entry_Names[]={"article","book"};
const char *const Texbib_Field_Names[][3]={{"title","journal","year"},{"title","publisher","year"}};

/*
BibTeX Model that reads btparses AST tree.
*/
class BibTeXModel : public QAbstractTableModel{
	public:
	/*
	Root of the AST tree.
	*/
	AST *root;
	AST *last_entry;
	int number_of_entries=0;
	/*
	Constructor calculates metadata of BibTex AST root given.
	*/
	BibTeXModel(QObject *parent,AST *root) : QAbstractTableModel(parent){
		this->root=root;

		// Finding last citation and counting number of them.
		this->last_entry=root;
		while(this->last_entry->right){
			this->number_of_entries++;
			this->last_entry=bt_next_entry(this->root,this->last_entry);
		}
	}

	/*
	Qt interface to implement.
	*/
	int rowCount(const QModelIndex &parent = QModelIndex()) const override{
		return this->number_of_entries+1;
	}

	int columnCount(const QModelIndex & parent) const override{
		return 5;
	}

	QVariant data(const QModelIndex &index, int role) const override{

		// Some sanity checks that index is valid? and row is not more than entries.
		if(!index.isValid()) return QVariant();
		if(index.row() >= this->number_of_entries+1) return QVariant();

		// Check role is set to display.
		if(role == Qt::DisplayRole){

			// Get entry of row given.
			AST *iteentry = this->root;
			int i=0;
			while(i<index.row()){
				iteentry=bt_next_entry(this->root,iteentry);
				i++;
			}

			// Find correct entry name. Handle entry type and key separately.
			// Get entry key.
			if(index.column()==3){
				return QString(bt_entry_type(iteentry));
			}
			else if(index.column()==4){
				return QString(bt_entry_key(iteentry));
			}
			else{
				// Search the right column.
				char *columnname;
				AST *ite_column=NULL;
				int colindex=index.column();
				// Entry type has different fields in it.
				// Hence map type name to supported types
				// and maps fields from there.
				char *typestr=bt_entry_type(iteentry);
				int type;
				for(int i=TEXBIB_ENTRY_GENERIC_NO_SUPPORT-1;i>=0;i--){
					if(strcmp(typestr,Supported_BibTex_Entry_Names[i])==0){
						type=i;
						break;
					}
				}

				// Look for data for this column of type.
				// Ite is at the entry type so jump to code moving iteration.
				goto MOVE_ITERATION;
				do{
					if(strcmp(Texbib_Field_Names[type][colindex],columnname)==0){

						// Value actual queried?
						char *value;

						//TODO: Value iterator is to get the value from bt_next_value.
						//      There is some kind of list system so maybe some fields
						//      has to be constructed?
						AST *valuequery=NULL;

						//TODO: Values type handling needed at least for macros.
						bt_nodetype quriedtype;
						bt_next_value(ite_column,valuequery,&quriedtype,&value);

						return QString(value);
					}
					MOVE_ITERATION:
					ite_column=bt_next_field(iteentry,ite_column,&columnname);

				}while(ite_column);
				// Didn't find right column so return empty.
				return QVariant();
			}
		}
		else return QVariant();
	}

	QVariant headerData(int section, Qt::Orientation orientation,int role = Qt::DisplayRole) const override{
		if(role != Qt::DisplayRole) return QVariant();
		if (orientation == Qt::Horizontal)
			return QString(Column_Names[section]);
		else
			return QVariant(section);
	}

	Qt::ItemFlags flags(const QModelIndex &index) const override{
		return Qt::ItemIsEnabled;
	}
};
/*
Class to house Qt GUI objects with main window.
*/
class MainWindow : public QWidget{
	public:
	/*
	Menu bar of the window.
	*/
	QMenuBar *menu_bar;
	/*
	File menu. Includes saving action.
	*/
	QMenu *menu_file;
	QAction *save_act;
	/*
	Tab handler for different data tables.
	*/
	QTabWidget *tabs;
	/*
	Qt needs view to the data. Make view per tab.
	*/
	QTableView *per_cite_view;
	QTableView *per_citee_view;
	/*
	Model for Qt tables to bibTeX.
	*/
	BibTeXModel *tex_model;

	MainWindow(AST *root) : QWidget(nullptr){

		// Allocate layout for the main window.
		QVBoxLayout *layout = new QVBoxLayout(this);
		this->setLayout(layout);

		// Allocate and add sub widgets.
		this->menu_bar = new QMenuBar(this);
		this->menu_file = this->menu_bar->addMenu(tr("&File"));
		layout->addWidget(this->menu_bar);
		this->tabs = new QTabWidget(this);
		layout->addWidget(this->tabs);
		this->per_cite_view = new QTableView(this);
		this->tabs->addTab(this->per_cite_view,tr("&Per cite table"));
		this->per_citee_view = new QTableView(this);
		this->tabs->addTab(this->per_citee_view,tr("&Per citee table"));
		this->tabs->setTabPosition(QTabWidget::South);

		// Initialize the model.
		this->tex_model = new BibTeXModel(this,root);
		this->per_cite_view->setModel(this->tex_model);

		// Initialize actions.
		this->save_act = new QAction(tr("&Save"),this);
		this->menu_file->addAction(this->save_act);

		// Size the window.
		this->resize(700,700);
	};
};

/*
Start the application here. Give bibTeX file as a input to the system.
*/
int main(int argc,char *argv[]){
	// Application sh*t. Houses the mainloop and other general
	// initialization.
	QApplication app = QApplication(argc,argv);

	// If no cfg file was given do informer the user and do noting.
	if(argc==1){
		std::cout << "User should give file to open!" << std::endl;
		return 0;
	}

	// Read the bibTeX file. BTO_COLLAPSE removes excessive whitespace.
	bt_initialize();
	boolean status;
	AST *root=bt_parse_file(argv[1],0,&status);

	// Initialize the GUI.
	MainWindow mainwindow = MainWindow(root);
	mainwindow.show();

	//TODO: This gives thread control to Qt...
	int returnvalue=app.exec();

	// Cleanup.
	bt_free_ast(root);
	bt_cleanup();

	return returnvalue;
}
