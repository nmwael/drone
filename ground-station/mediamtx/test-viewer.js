#!/usr/bin/env node
/**
 * HTML Validator for stanag-s3-simulator viewer.html
 * Checks for common HTML issues that cause runtime errors
 */

const fs = require('fs');
const path = require('path');

const VIEWER_HTML = '/workspaces/drone/ground-station/mediamtx/viewer.html';

function readFile(path) {
  try {
    return fs.readFileSync(path, 'utf8');
  } catch (e) {
    console.error(`Error: Cannot read file: ${path}`);
    console.error(`  ${e.message}`);
    process.exit(1);
  }
}

function extractHtmlComments(html) {
  return html.replace(/<!--[\s\S]*?-->/g, '');
}

function extractScriptContent(html) {
  const scripts = [];
  const regex = /<script[^>]*>([\s\S]*?)<\/script>/gi;
  let match;
  while ((match = regex.exec(html)) !== null) {
    scripts.push(match[1]);
  }
  return scripts.join('\n');
}

function extractElementIds(html) {
  const ids = [];
  const idRegex = /id="([^"]+)"/g;
  let match;
  while ((match = idRegex.exec(html)) !== null) {
    ids.push(match[1]);
  }
  return ids;
}

function findElementUsagesInJs(jsCode) {
  const usages = [];

  // Match document.getElementById('id')
  const getElementByIdRegex = /document\.getElementById\(['"]([^'"]+)['"]\)/g;
  let match;
  while ((match = getElementByIdRegex.exec(jsCode)) !== null) {
    usages.push({
      id: match[1],
      type: 'getElementById',
      line: getLineNumber(jsCode, match.index)
    });
  }

  // Match document.querySelector('#id')
  const querySelectorRegex = /document\.querySelector\(['"](#[^'"]+)['"]\)/g;
  while ((match = querySelectorRegex.exec(jsCode)) !== null) {
    usages.push({
      id: match[1].slice(1), // Remove the #
      type: 'querySelector',
      line: getLineNumber(jsCode, match.index)
    });
  }

  return usages;
}

function getLineNumber(str, index) {
  return str.substring(0, index).split('\n').length;
}

function checkForCommonIssues(html) {
  const issues = [];

  // Check for unclosed div tags (rough check)
  const divOpenCount = (html.match(/<div\b[^>]*>/g) || []).length;
  const divCloseCount = (html.match(/<\/div>/g) || []).length;
  if (divOpenCount !== divCloseCount) {
    issues.push({
      type: 'ERROR',
      message: `Unbalanced div tags: ${divOpenCount} opening, ${divCloseCount} closing`
    });
  }

  // Check for unclosed script tags
  const scriptOpenCount = (html.match(/<script[^>]*>/g) || []).length;
  const scriptCloseCount = (html.match(/<\/script>/g) || []).length;
  if (scriptOpenCount !== scriptCloseCount) {
    issues.push({
      type: 'ERROR',
      message: `Unbalanced script tags: ${scriptOpenCount} opening, ${scriptCloseCount} closing`
    });
  }

  return issues;
}

function main() {
  console.log('='.repeat(60));
  console.log('HTML Validation Report');
  console.log('='.repeat(60));

  const html = readFile(VIEWER_HTML);
  console.log(`File: ${VIEWER_HTML}`);
  console.log(`Size: ${html.length} bytes`);
  console.log('');

  // Check HTML structure
  console.log('\n--- HTML Structure Validation ---');
  const issues = checkForCommonIssues(html);
  if (issues.length === 0) {
    console.log('No structural HTML errors found');
  } else {
    issues.forEach(i => console.log(`  ${i.type}: ${i.message}`));
  }

  // Extract all element IDs from HTML
  const htmlIds = extractElementIds(html);
  console.log(`\nFound ${htmlIds.length} elements with IDs in HTML`);

  // Extract all element ID usages from JavaScript
  const jsCode = extractScriptContent(html);
  const jsUsages = findElementUsagesInJs(jsCode);
  console.log(`Found ${jsUsages.length} element references in JavaScript`);

  // Check each JS-referenced ID exists in HTML
  const htmlIdSet = new Set(htmlIds);
  const missingIds = [];
  const usedIds = new Set();

  for (const usage of jsUsages) {
    usedIds.add(usage.id);
    if (!htmlIdSet.has(usage.id)) {
      missingIds.push({
        id: usage.id,
        type: usage.type,
        line: usage.line
      });
    }
  }

  if (missingIds.length === 0) {
    console.log('All element IDs referenced in JavaScript exist in HTML');
  } else {
    console.log(`\n  ${missingIds.length} element ID(s) referenced in JavaScript but missing from HTML:`);
    missingIds.forEach(m => {
      console.log(`   Line ${m.line}: #${m.id} (${m.type})`);
    });
  }

  // Check for unused IDs in HTML (not necessarily an error, but good to know)
  const usedIdSet = new Set(usedIds);
  const unusedIds = htmlIds.filter(id => !usedIdSet.has(id));
  if (unusedIds.length > 0) {
    console.log(`  ${unusedIds.length} element ID(s) in HTML but not referenced in JavaScript:`);
    unusedIds.forEach(id => console.log(`   #${id}`));
  }

  console.log('');
  console.log('='.repeat(60));
  console.log('Validation Complete');
  console.log('='.repeat(60));
}

main();