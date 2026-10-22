<?php

/**
 * @file
 * A minimal Word (.docx) writer: paragraphs with a heading style, bold and plain runs, page breaks and lines.
 *
 * A block is an array with:
 * - 'runs': list of array('text' => string, 'bold' => bool); text is plain, newlines become line breaks.
 * - 'style': optional 'Heading1', 'Heading2', 'Heading3' or 'Footer'.
 * - 'page_break': optional, starts the paragraph on a new page.
 * - 'rule': optional, an empty paragraph drawn as a horizontal line.
 */

/**
 * Plain text of user-entered or stored text: tags removed, entities decoded, at most one empty line in a row; spaces are kept, so runs join as written.
 */
function ammo_docx_plain($text) {
  $text = decode_entities(strip_tags(preg_replace('/<br\s*\/?>/i', "\n", (string) $text)));
  $text = str_replace(array("\r\n", "\r"), "\n", $text);
  return trim(preg_replace("/\n{3,}/", "\n\n", $text), "\n");
}

/**
 * A block of one plain run.
 */
function ammo_docx_block($text, $style = NULL) {
  return array('runs' => array(array('text' => $text)), 'style' => $style);
}

/**
 * A block with a bold label followed by a plain value.
 */
function ammo_docx_label_block($label, $value) {
  return array('runs' => array(array('text' => $label, 'bold' => TRUE), array('text' => ' ' . $value)));
}

/**
 * The WordprocessingML of one block.
 */
function ammo_docx_paragraph(array $block) {
  $properties = '';
  if (!empty($block['style'])) {
    $properties .= '<w:pStyle w:val="' . $block['style'] . '"/>';
  }
  if (!empty($block['page_break'])) {
    $properties .= '<w:pageBreakBefore/>';
  }
  if (!empty($block['rule'])) {
    $properties .= '<w:pBdr><w:bottom w:val="single" w:sz="6" w:space="1" w:color="999999"/></w:pBdr>';
  }
  $xml = '<w:p>' . ($properties ? '<w:pPr>' . $properties . '</w:pPr>' : '');
  foreach (isset($block['runs']) ? $block['runs'] : array() as $run) {
    $xml .= '<w:r>' . (!empty($run['bold']) ? '<w:rPr><w:b/></w:rPr>' : '');
    foreach (explode("\n", ammo_docx_plain($run['text'])) as $i => $line) {
      $xml .= ($i ? '<w:br/>' : '') . '<w:t xml:space="preserve">' . htmlspecialchars($line, ENT_XML1 | ENT_QUOTES, 'UTF-8') . '</w:t>';
    }
    $xml .= '</w:r>';
  }
  return $xml . '</w:p>';
}

/**
 * The bytes of a Word document with the blocks and an optional footer line on every page, or FALSE when the zip cannot be written.
 */
function ammo_docx_build(array $blocks, $footer = '') {
  $body = '';
  foreach ($blocks as $block) {
    $body .= ammo_docx_paragraph($block);
  }
  $w = 'xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships"';
  $font = '<w:rFonts w:ascii="Arial" w:hAnsi="Arial" w:cs="Arial"/>';
  $heading = function ($id, $name, $size, $level) use ($font) {
    return '<w:style w:type="paragraph" w:styleId="' . $id . '"><w:name w:val="' . $name . '"/><w:basedOn w:val="Normal"/><w:next w:val="Normal"/><w:qFormat/><w:pPr><w:keepNext/><w:spacing w:before="240" w:after="120"/><w:outlineLvl w:val="' . $level . '"/></w:pPr><w:rPr>' . $font . '<w:b/><w:sz w:val="' . $size . '"/></w:rPr></w:style>';
  };
  $parts = array(
    '[Content_Types].xml' => '<?xml version="1.0" encoding="UTF-8" standalone="yes"?><Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types"><Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/><Default Extension="xml" ContentType="application/xml"/><Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/><Override PartName="/word/styles.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/><Override PartName="/word/footer1.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/></Types>',
    '_rels/.rels' => '<?xml version="1.0" encoding="UTF-8" standalone="yes"?><Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/document.xml"/></Relationships>',
    'word/_rels/document.xml.rels' => '<?xml version="1.0" encoding="UTF-8" standalone="yes"?><Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/><Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer" Target="footer1.xml"/></Relationships>',
    'word/styles.xml' => '<?xml version="1.0" encoding="UTF-8" standalone="yes"?><w:styles ' . $w . '><w:docDefaults><w:rPrDefault><w:rPr>' . $font . '<w:sz w:val="22"/><w:lang w:val="nl-NL"/></w:rPr></w:rPrDefault><w:pPrDefault><w:pPr><w:spacing w:after="120" w:line="276" w:lineRule="auto"/></w:pPr></w:pPrDefault></w:docDefaults><w:style w:type="paragraph" w:default="1" w:styleId="Normal"><w:name w:val="Normal"/><w:qFormat/></w:style>' . $heading('Heading1', 'heading 1', 36, 0) . $heading('Heading2', 'heading 2', 30, 1) . $heading('Heading3', 'heading 3', 24, 2) . '<w:style w:type="paragraph" w:styleId="Footer"><w:name w:val="footer"/><w:basedOn w:val="Normal"/><w:pPr><w:spacing w:after="0"/></w:pPr><w:rPr><w:color w:val="666666"/><w:sz w:val="18"/></w:rPr></w:style></w:styles>',
    'word/footer1.xml' => '<?xml version="1.0" encoding="UTF-8" standalone="yes"?><w:ftr ' . $w . '>' . ammo_docx_paragraph(array('runs' => array(array('text' => $footer)), 'style' => 'Footer')) . '</w:ftr>',
    'word/document.xml' => '<?xml version="1.0" encoding="UTF-8" standalone="yes"?><w:document ' . $w . '><w:body>' . $body . '<w:sectPr><w:footerReference w:type="default" r:id="rId2"/><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1418" w:right="1418" w:bottom="1418" w:left="1418" w:header="709" w:footer="709" w:gutter="0"/></w:sectPr></w:body></w:document>',
  );
  $path = drupal_tempnam('temporary://', 'ammo-docx-');
  $zip = new ZipArchive();
  if (!$path || $zip->open(drupal_realpath($path), ZipArchive::CREATE | ZipArchive::OVERWRITE) !== TRUE) {
    return FALSE;
  }
  foreach ($parts as $name => $content) {
    $zip->addFromString($name, $content);
  }
  $zip->close();
  $bytes = file_get_contents($path);
  drupal_unlink($path);
  return $bytes;
}
