/*
< Tool for managing bibTeX file as reference.
<
< Relevant Qt tutorials:
<   https://doc.qt.io/qt-6/qtsql-cachedtable-example.html
<   https://doc.qt.io/qt-6/qtwidgets-mainwindows-menus-example.html
<   https://doc.qt.io/qt-6/qtwidgets-dialogs-tabdialog-example.html
<   https://doc.qt.io/qt-6/modelview.html
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
#include "btparseWrite.h"

/*
String constants for column names.
*/
const char * const Column_Names[]={"Name","Publication/Journal","Year","Number/volume","Authors","Entry type","Entry key","Reference list"};

/*
Types of entries supported and field mappings to columns.
Each supported entry has it's own fields.
*/
enum Supported_BibTex_Entries{
	TEXBIB_ENTRY_ARTICLE=0,
	TEXBIB_ENTRY_BOOK=1,
	TEXBIB_ENTRY_GENERIC_NO_SUPPORT
};
const char *const Supported_BibTex_Entry_Types[]={"article","book"};
const char *const Texbib_Field_Names[][5]={{"title","journal","year","number","author"},{"title","publisher","year","volume","author"}};

/*
* For additions to normal BibTeX fields is tools own field listing references.
* Variable is field name in the BibTeX file.
*/
const char *Literature_Review_Reference_List_Field_Name="LRreflist";

/*
Find entry of given row.
*/
static AST *findRowEntry(const QModelIndex &index, AST *root){
	// Easiest way is to start at root and then find next entry
	// until iteration index is the same as given model index.
	// No point manually use iteentry->right because bt_next_entry
	// does not brake later. Also there could be non-entry types
	// technically so we do not need to make check for entry type.
	AST *iteentry = root;
	int i=0;
	while(i<index.row()){
		iteentry=bt_next_entry(root,iteentry);
		i++;
	}
	return iteentry;
}

static AST *findField(const char *fieldname, AST *entry){
	AST *itecolumn=NULL;
	char *foundfieldname;
	// Look for data for this column of type.
	// Ite is at the entry type so jump to code moving iteration.
	goto MOVE_ITERATION;
	do{
		if(strcmp(fieldname,foundfieldname)==0){
			return itecolumn;
		}
		MOVE_ITERATION:
		itecolumn=bt_next_field(entry,itecolumn,&foundfieldname);

	}while(itecolumn);

	// Function didn't find a field so return
	// null as a indicator.
	return NULL;
}

static QVariant findFieldValue(const char *fieldname, AST *entry){

	AST *field;
	if((field=findField(fieldname,entry))!=NULL){
		// Value actual queried?
		char *value;

		//TODO: Value iterator is to get the value from bt_next_value.
		//      There is some kind of list system so maybe some fields
		//      has to be constructed?
		AST *valuequery=NULL;

		//TODO: Values type handling needed at least for macros.
		bt_nodetype quriedtype;
		bt_next_value(field,valuequery,&quriedtype,&value);

		return QString(value);
	}
	// Didn't find right column so return empty.
	return QVariant();
}

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
	BibTeXModel(QObject *parent, AST *root) : QAbstractTableModel(parent){
		this->root = root;

		// Finding last citation and counting number of them.
		// Needed to tell number of rows to table view.
		this->last_entry = NULL;
		this->last_entry = bt_next_entry(this->root,NULL);
		// If last entry is NULL it means empty BibTeX file was give.
		if(this->last_entry){
			while(this->last_entry!=NULL && this->last_entry->right){
				this->number_of_entries++;
				this->last_entry = bt_next_entry(this->root,this->last_entry);
			}
			// Loop didn't add last entry.
			this->number_of_entries++;
		}
		else{
			// Since it happens and right, which is to next entry, is zero
			// offset position in the AST structure we can use this to make
			// phantom first entry by root address. Since this assumes
			// something about underline structure check the assumption.
			static_assert(offsetof(AST,right)==0,"Code assumes that \"AST\" structure has member \"right\" on zero offset.");
			this->last_entry = (AST *)&this->root;
		}
	}

	/*
	Qt interface to implementation.
	ModelView needs to be told amount of rows and columns.
	Function data returns value for given model index.
	Function setdata edits given model index.
	Function flags tells what role model index has.
	*/
	int rowCount(const QModelIndex &parent = QModelIndex()) const override{
		return this->number_of_entries;
	}

	int columnCount(const QModelIndex & parent) const override{
		// Number of columns in the data.
		// This is constant amount because program is interested in
		// finite amount of data per BibTeX entry.
		return 8;
	}

	QVariant data(const QModelIndex &index, int role) const override{

		// Some sanity checks that index is valid? and row is not more than entries.
		if(!index.isValid()) return QVariant();
		if(index.row() >= this->number_of_entries+1) return QVariant();

		// Check role is set to display.
		if(role == Qt::DisplayRole || role == Qt::EditRole){

			// Get the entry.
			AST *iteentry=findRowEntry(index,this->root);

			// Find correct entry name. Handle entry type and key separately
			// which then leads to special handling of reference list.
			// This is because btparse gives separate functions for these.
			if(index.column()==5){
				return QString(bt_entry_type(iteentry));
			}
			else if(index.column()==6){
				return QString(bt_entry_key(iteentry));
			}
			else if(index.column()==7){
				return findFieldValue(Literature_Review_Reference_List_Field_Name,iteentry);
			}
			else{

				// Find entry type map it to id and use it and column index to
				// get correct bibTeX field name for this column.
				int colindex = index.column();

				// Entry type has different fields in it.
				// Hence map type name to supported types
				// and maps fields from there.
				char *typestr = bt_entry_type(iteentry);
				int type = TEXBIB_ENTRY_GENERIC_NO_SUPPORT;
				for(int i=TEXBIB_ENTRY_GENERIC_NO_SUPPORT-1;i>=0;i--){
					if(strcmp(typestr,Supported_BibTex_Entry_Types[i])==0){
						type = i;
						break;
					}
				}

				// This evaluates true when ever supported
				// type is not found on the list.
				// TODO: Make add generic type field names and remove this.
				if(type==TEXBIB_ENTRY_GENERIC_NO_SUPPORT) return QVariant();

				// Find field value given by type and column index mapping.
				return findFieldValue(Texbib_Field_Names[type][colindex],iteentry);
			}
		}
		else return QVariant();
	}

	bool setData(const QModelIndex &index, const QVariant &value, int role) override{
		if(role == Qt::EditRole && this->checkIndex(index)){
			// Editable role hence implement the editing.
			// However, field does not necessarily exists. So
			// if it does not create the field. If it does just
			// edit the field.

			// By default QVariant direct data access method does not give readable result.
			// Hence, experimental result to get UTF8 output from variant.
			char *utf8str = value.toString().toUtf8().data();

			// Edit this row's field so find the entry.
			AST* entry = findRowEntry(index,this->root);

			// If field does not exist add the field.
			AST *field;
			if((field = findField(Literature_Review_Reference_List_Field_Name,entry)) != NULL){
				bt_set_field_value(field,utf8str);
			}
			else bt_add_field(entry,Literature_Review_Reference_List_Field_Name,utf8str);

			return true;
		}
		// Does not have editable role so return false.
		return false;
	}

	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override{
		if(role != Qt::DisplayRole) return QVariant();
		if (orientation == Qt::Horizontal)
			return QString(Column_Names[section]);
		else
			return QVariant(section);
	}

	Qt::ItemFlags flags(const QModelIndex &index) const override{
		// If not valid index this enabled?
		if(!index.isValid()) return Qt::ItemIsEnabled;

		// This column is citations reference id list which is editable.
		if(index.column()==7){
			Qt::ItemFlags returnflag = QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
			return returnflag;
		}
		// Just call parent's flags if column is uneditable.
		return QAbstractItemModel::flags(index);
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
	QAction *save_as_act;
	QAction *add_citation_act;
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

	/*
	Methods for the menu actions.
	*/
	private slots:
	/*
	Save current file to same location.
	*/
	void save(){
		const char *filepath = "TEST.bib";
		bt_save_file(filepath,this->tex_model->root);
	}

	public:
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
		// Column sizes are manually tested to this size.
		this->tex_model = new BibTeXModel(this,root);
		this->per_cite_view->setModel(this->tex_model);
		this->per_cite_view->setWordWrap(true);
		this->per_cite_view->setColumnWidth(0,400);
		this->per_cite_view->setColumnWidth(1,400);
		this->per_cite_view->setColumnWidth(2,40);
		this->per_cite_view->setColumnWidth(3,100);
		this->per_cite_view->setColumnWidth(4,100);
		this->per_cite_view->setColumnWidth(5,80);
		this->per_cite_view->setColumnWidth(7,120);

		// Initialize actions.
		this->save_act = new QAction(tr("&Save"),this);
		this->menu_file->addAction(this->save_act);
		connect(this->save_act,&QAction::triggered,this,&MainWindow::save);
		this->save_as_act = new QAction(tr("&Save as"),this);
		this->menu_file->addAction(this->save_as_act);
		this->add_citation_act = new QAction(tr("&Add citation"),this);
		this->menu_file->addAction(this->add_citation_act);

		// Size the window.
		this->resize(1600,1000);
	};
};

/*
Start the application here. Give bibTeX file as a input to the system.
*/
int main(int argc, char *argv[]){
	// Application sh*t. Houses the mainloop and other general
	// initialization.
	QApplication app = QApplication(argc,argv);

	// If no cfg file was given do informer the user and do noting.
	if(argc==1){
		std::cout << "User should give file to open!" << std::endl;
		return 0;
	}

	// Read the bibTeX file.
	bt_initialize();
	boolean status;
	AST *root = bt_parse_file(argv[1],0,&status);
	if(status==FALSE){
		std::cout << "Parsing bibTeX file failed!"<<std::endl;
	}

	// Initialize the GUI.
	MainWindow mainwindow = MainWindow(root);
	mainwindow.show();

	//TODO: This gives thread control to Qt...
	int returnvalue = app.exec();

	// Cleanup.
	bt_free_ast(root);
	bt_cleanup();

	return returnvalue;
}
