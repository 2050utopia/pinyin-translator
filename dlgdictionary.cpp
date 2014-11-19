// Copyright (c) 2010-2014 Sami Väisänen, Ensisoft 
//
// http://www.ensisoft.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#include "config.h"
#include "warnpush.h"
#  include <QSettings>
#include "warnpop.h"

#include "dlgdictionary.h"
#include "dlgword.h"
#include "dictionary.h"

namespace pime
{

class DlgDictionary::TableModel : public QAbstractTableModel
{
public:
    TableModel(dictionary& dic) : dic_(dic)
    {}

    virtual QVariant data(const QModelIndex& index, int role) const override
    {
        const auto row = index.row();
        const auto col = index.column();

        if (role == Qt::DisplayRole)
        {
            const auto& word = words_[row];
            switch (col)
            {
                case 0: return word.chinese;
                case 1: return word.pinyin;
                case 2: return word.description;
            }
        }
        return QVariant();
    }
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (role == Qt::DisplayRole)
        {
            switch (section)
            {
                case 0: return "Chinese";
                case 1: return "Pinyin";
                case 2: return "Definition";
            }        
        }
        return QVariant();        
    }
    virtual int rowCount(const QModelIndex&) const override
    {
        return (int)words_.size();
    }
    virtual int columnCount(const QModelIndex&) const override
    {
        return 3;
    }

    void update(const QString& key)
    {
        if (key.isEmpty())
            words_ = dic_.flatten();
        else words_ = dic_.lookup(key);

        reset();

        search_ = key;

    }
    std::size_t wordCountTotal() const
    { return dic_.wordCount(); }

    std::size_t wordCountShown() const 
    { return words_.size(); }

    void store(dictionary::word& w)
    {
        if (dic_.store(w))
        {
            if (search_.isEmpty())
                words_ = dic_.flatten();
            else words_ = dic_.lookup(search_);

            const auto first = index(0, 0);
            const auto last  = index((int)words_.size(), 3);
            emit dataChanged(first, last);
        }
        else
        {
            if (search_.isEmpty())
                words_ = dic_.flatten();
            else words_ = dic_.lookup(search_);
            
            reset();
        }
    }

    void deleteWords(QModelIndexList& indices)
    {}

    const dictionary::word& getWord(std::size_t i)
    {
        Q_ASSERT(i < words_.size());
        return words_[i];
    }


private:
    dictionary& dic_;
    std::vector<dictionary::word> words_;
private:
    QString search_;
};


DlgDictionary::DlgDictionary(QWidget* parent, dictionary& dic) : QDialog(parent), model_(new TableModel(dic))
{
    ui_.setupUi(this);
    ui_.tableView->setModel(model_.get());

    QSettings settings("Ensisoft", "Pime");
    const auto wwidth = settings.value("dictionary-window/width", width()).toInt();
    const auto wheight = settings.value("dictionary-window/height", height()).toInt();
    const auto xpos = settings.value("dictionary-window/xpos", x()).toInt();
    const auto ypos = settings.value("dictionary-window/ypos", y()).toInt();

    move(xpos, ypos);
    resize(wwidth, wheight);

    updateTable("");
}


DlgDictionary::~DlgDictionary()
{}

void DlgDictionary::updateTable(const QString& search)
{
    model_->update(search);
    const auto total = model_->wordCountTotal();
    const auto shown = model_->wordCountShown();

    ui_.lblCount->setText(QString("Total %1 words. Currently showing %2 words.")
        .arg(total)
        .arg(shown));
}

void DlgDictionary::on_btnAdd_clicked()
{
    DlgWord dlg(this, "");
    if (dlg.exec() == QDialog::Rejected)
        return;

    dictionary::word word;
    word.key         = dlg.key();
    word.chinese     = dlg.chinese();
    word.description = dlg.desc();
    word.pinyin      = dlg.pinyin();
    word.tags        = 0;
    
    model_->store(word);
}

void DlgDictionary::on_btnDel_clicked()
{
    auto rows = ui_.tableView->selectionModel()->selectedRows();
    if (rows.isEmpty())
        return;
    
    model_->deleteWords(rows);
}

void DlgDictionary::on_btnEdit_clicked()
{
    auto rows = ui_.tableView->selectionModel()->selectedRows();
    if (rows.isEmpty())
        return;

    for (auto i=0; i<rows.size(); ++i)
    {
        auto row = rows[i].row();
        auto word = model_->getWord(row);
        DlgWord dlg(this, word.key, 
            word.pinyin,
            word.chinese,
            word.description);
        if (dlg.exec() == QDialog::Rejected)
            continue;

        word.pinyin  = dlg.pinyin();
        word.chinese = dlg.chinese();
        word.description = dlg.desc();
        model_->store(word);
    }
}

void DlgDictionary::on_btnClose_clicked()
{
    QSettings settings("Ensisoft", "Pime");
    settings.setValue("dictionary-window/width", width());
    settings.setValue("dictionary-window/height", height());
    settings.setValue("dictionary-window/xpos", x());
    settings.setValue("dictionary-window/ypos", y());

    close();
}

void DlgDictionary::on_tableView_doubleClicked(const QModelIndex&)
{
    on_btnEdit_clicked();
}

} // pime