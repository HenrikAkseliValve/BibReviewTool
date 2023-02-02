/*
<
< Relevant Qt tutorials:
<   https://doc.qt.io/qt-6/qtsql-cachedtable-example.html
<   https://doc.qt.io/qt-6/qtwidgets-mainwindows-menus-example.html
<   https://doc.qt.io/qt-6/qtwidgets-dialogs-tabdialog-example.html
*/

#include <iostream>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableView>
#include <QtWidgets/QVBoxLayout>
#include <btparse.h>

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
	BibTeXModel(QObject *parent,AST *root) : QAbstractTableModel(parent){
		this->root=root;

		// Finding last citation and counting number of them.
		this->last_entry=root;
		while(this->last_entry->down){
			this->number_of_entries++;
			this->last_entry=bt_next_entry(this->root,this->last_entry);
		}

		std::cout << "\n" << this->last_entry->text<< "\n" << std::endl;
	}

	/*
	Qt interface to implement.
	*/
	int rowCount(const QModelIndex &parent = QModelIndex()) const override{
		return 1;
	}
	int columnCount(const QModelIndex & parent) const override{
		return 1;
	}
	QVariant data(const QModelIndex &index, int role) const override{
		return QVariant("1");
	}
	QVariant headerData(int section, Qt::Orientation orientation,int role = Qt::DisplayRole) const override{
		return QVariant("2");
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

		// Initialize actions.
		this->save_act = new QAction(tr("&Save"),this);
		this->menu_file->addAction(this->save_act);

		// Size the window.
		this->resize(600,600);
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
