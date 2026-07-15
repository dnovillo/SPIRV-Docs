#!/usr/bin/env python3
# Copyright 2015-2026 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

import requests
from bs4 import BeautifulSoup
import sys

def check_broken_links(file):
    with open(file, 'r') as fd:
        soup = BeautifulSoup(fd, features="html.parser")
    ids = set([tag['id'] for tag in soup.find_all(lambda tag: tag.has_attr('id'))])
    hrefs = set([tag['href'] for tag in soup.find_all("a", href=True)])
    dead_anchors = set()
    broken_links = set()
    for link in hrefs:
        if link.startswith('#'):
            if not link[1:] in ids:
                dead_anchors.add(link)
        else:
            try:
                r = requests.head(link, timeout=10)
                if r.status_code == 404:
                    broken_links.add(link)
            except requests.exceptions.RequestException as e:
                print("Warning: could not check '{}': {}".format(link, e))


    for a in dead_anchors:
        print("Dead anchor ref '{}'".format(a))
    for l in broken_links:
        print("Dead link '{}'".format(l))

    return len(dead_anchors) != 0 or len(broken_links) != 0

def parse_args():
    import argparse
    parser = argparse.ArgumentParser(description='check for dead links in a html file')

    parser.add_argument('htmls',
                                                nargs='+',
                        help='Path to the html files.')
    return parser.parse_args()

args = parse_args()

has_error = False
for file in args.htmls:
    print("------ processing {}".format(file))
    has_error = check_broken_links(file) or has_error

if has_error:
    sys.exit(1)
