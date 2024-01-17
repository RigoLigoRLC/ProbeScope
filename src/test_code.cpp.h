

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
