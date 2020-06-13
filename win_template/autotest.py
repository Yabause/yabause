#------------------------------------------------------------------------------
# Automated Test framework for Yaba Sanshrio.
# デグレチェックの自動化
#                                                                   devMiyax
#
# How to use
# 1) テストケースと正解データの準備
#    * YabaSansiroの実行
#    * Menuの"Emulation"->"Record"を選択して記録開始
#    * 正解画像を取りたいところでCtrl+Pを押す
#    * Menuの"Emulation"->"Record"を選択して記録終了
# 2) テストケースの記述
#    * autotest.pyのある場所にtestcase.jsonファイルを作成する
#    * 以下の内容のjsonデータを記述する
#    {
#      "exename":<テストしたい実行ファイルのパス>,
#      "testcase":[
#         { "iso":<テストしたいゲームのISOイメージのパス>,
#           "id":<レコードデータのあるパス> }
#      ]
#    }
# 3) テスト実行
#    * autotest.pyを実行する
#    * 一通りテストが終わると結果が表示される
#    * 画像が比較され同一の場合のスコアは1.00となる
#------------------------------------------------------------------------------

import subprocess
from subprocess import PIPE
import cv2
import numpy as np
import os
from yattag import Doc
import webbrowser
from datetime import datetime
import json
import time

class TestCase:
  def __init__(self, iso, id):
    self.iso = iso
    self.id = id
    self.result = []
    self.score=0.0
    self.time=0.0

  def exec(self):

    idpath = testpath + "/" + self.id

    if(os.path.exists(self.iso) != True):
      print( self.iso + " does not exist!")
      return -1

    if(os.path.exists(idpath) != True):
      print( idpath + " does not exist!")
      return -1

    command = exename + " -i " + "\"" + self.iso + "\"" + " -a -p " + idpath
    print(command)
    start = time.time()
    proc = subprocess.run(command, shell=True, stdout=PIPE, stderr=PIPE, text=True)
    self.time = time.time() - start
    print(proc.stdout)
    for file in sorted(os.listdir(idpath)):
        if file.endswith(".png"):
          comp = 0.0
          im = cv2.imread( idpath + "/" + file )
          if im is None:
            print( self.id + "/" + file + " does not exist!")
            return -1
          im = cv2.resize(im, IMG_SIZE)
          target_hist = cv2.calcHist([im], [0], None, [256], [0, 256])
          if(os.path.exists( idpath + "out/" + file)):
            im_after = cv2.imread( idpath + "out/" + file )
            if not im_after is None:
              im_after = cv2.resize(im_after, IMG_SIZE)
              comparing_hist = cv2.calcHist([im_after], [0], None, [256], [0, 256])
              comp = cv2.compareHist(target_hist, comparing_hist, 0)
              self.score += comp
          row = [ self.id + "/" + file, self.id + "out/" + file, comp ]
          self.result.append(row)
          print(comp)
    return 0

def ydump_table(headings, rows, **kwargs):
  doc, tag, text, line = Doc().ttl()
  with tag('table', klass="table table-bordered", **kwargs):
    with tag('tr'):
      for x in headings:
        line('th', str(x))
      for row in rows:
        k = ""
        if row[2] < 0.5:
          k = "danger"
        with tag('tr', klass=k):
          with tag('td'):
            doc.stag('img', src=row[0], klass="photo", width="320")
          with tag('td'):
            doc.stag('img', src=row[1], klass="photo",width="320")
          line('td', "{:.2f}".format(row[2]))
          #//for x in row:
          #  line('td', str(x)) 
  return doc.getvalue()


def main():
  global exename
  current_hash = subprocess.getoutput("git rev-parse HEAD")
  now = datetime.now()
  dt_string = now.strftime("%d/%m/%Y %H:%M:%S")
  print("Test is starting for " + current_hash.split()[0] + " at " + dt_string)

  if(os.path.exists('yabtest/testcase.json') != True):
    print( "testcase.json does not exist!")
    exit(-1)

  tests = []
  with open('yabtest/testcase.json') as json_file:
    data = json.load(json_file)
    exename = data['exename']
    for p in data['testcase']:
      tests.append(TestCase( p['iso'],p['id']))

  # Check Before test
  if(os.path.exists(exename) != True):
    print( exename + " does not exist!")
    exit(-1)

  totalscore = 0.0
  totaltime = 0.0
  for test in tests:
    test.exec()
    totalscore += test.score
    totaltime += test.time


  doc, tag, text, line = Doc().ttl()
  doc.asis('<!DOCTYPE html>')
  with tag('html'):
    with tag('head'):
      doc.asis('<meta charset="utf-8">')
      doc.asis('<meta name="viewport" content="width=device-width, initial-scale=1">')
      #doc.asis('<link rel="stylesheet" href="http://maxcdn.bootstrapcdn.com/bootstrap/3.3.6/css/bootstrap.min.css">')
      doc.asis('<link rel="stylesheet" href="https://bootswatch.com/3/sandstone/bootstrap.css">')
      with tag('script', src="https://ajax.googleapis.com/ajax/libs/jquery/1.12.0/jquery.min.js"):
        pass
      with tag('script', src="http://maxcdn.bootstrapcdn.com/bootstrap/3.3.6/js/bootstrap.min.js"):
        pass    
    with tag('body'):
      with tag('div', klass='container'): 
        with tag('H1'):
          text("Test Total Score:","{:.2f}".format(totalscore)) 
          doc.stag('br')
          text("Test Total Time:","{:.2f}".format(totaltime) + "[sec]") 

        with tag('p'):
          text(str(current_hash.split()[0]))
          doc.stag('br')
          text(dt_string)
        for test in tests:
          with tag('div', klass='title'):
            text("ID:",test.id)
            doc.stag('br')
            text("Duration:{:.2f}".format(test.time) + "[sec]" )
          doc.asis(ydump_table( headings, test.result ))
  
  
  result = doc.getvalue()

  with open( testpath + "/result.html", "w") as file:
      file.write(result)

  url = "file://" + os.getcwd() + "/"+testpath+"/result.html"
  webbrowser.open(url,new=2)


IMG_SIZE = (320, 240)
testpath = "yabtest"
exename = ""
headings = ["GT","Result","score"]

if __name__ == "__main__":
  main()
