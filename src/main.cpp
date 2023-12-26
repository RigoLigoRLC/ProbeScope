
#include <QApplication>
#include <QLabel>
#include <QMainWindow>
#include <QStatusBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <psprobe/families.h>

void InsertFamilies(QTreeWidget &, size_t &, size_t &);

int main(int argc, char **argv) {
  QApplication app(argc, argv);

  QMainWindow window;
  QTreeWidget tree(&window);
  QStatusBar statusBar;
  QLabel statusLabel;

  size_t familyCount = 0, variantCount = 0;
  InsertFamilies(tree, familyCount, variantCount);
  statusLabel.setText(QString("Families: %1, Chip variants: %2")
                          .arg(familyCount)
                          .arg(variantCount));

  statusBar.addWidget(&statusLabel);

  window.setCentralWidget(&tree);
  window.setStatusBar(&statusBar);
  window.show();

  return app.exec();
}

void InsertFamilies(QTreeWidget &tree, size_t &familyCount,
                    size_t &variantCount) {
  void *families;
  psprobe_status status = 0;

  size_t families_size = 0;
  status = psprobe_families_get(&families, &families_size);
  if (status)
    return;

  for (size_t i = 0; i < families_size; i++) {
    char *family_name;
    size_t name_length;
    QTreeWidgetItem *item;

    status = psprobe_families_get_name(families, i, &family_name, &name_length);
    if (status)
      continue;

    // Get variants names
    size_t variant_count;
    status = psprobe_families_get_variant_count(families, i, &variant_count);
    if (status)
      continue;

    item = new QTreeWidgetItem(&tree);
    item->setText(0, QString::fromUtf8(family_name, int(name_length)));
    familyCount++;

    for (size_t j = 0; j < variant_count; j++) {
      char *variant_name;
      size_t variant_name_length;
      status = psprobe_families_get_variant_name(families, i, j, &variant_name,
                                                 &variant_name_length);
      if (status)
        continue;

      QTreeWidgetItem *variant_item = new QTreeWidgetItem(item);
      variant_item->setText(
          0, QString::fromUtf8(variant_name, int(variant_name_length)));
      variantCount++;
    }
  }

  status = psprobe_families_destroy(families);
}
