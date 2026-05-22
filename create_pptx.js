const pptxgen = require("pptxgenjs");

const pres = new pptxgen();
pres.layout = "LAYOUT_16x9";
pres.author = "入党思想汇报";
pres.title = "入党思想汇报";

// === COLOR PALETTE ===
const C = {
  red: "C41E3A",
  darkRed: "8B1A2B",
  gold: "D4A853",
  lightBg: "FFFAF5",
  white: "FFFFFF",
  textDark: "2D2D2D",
  textMedium: "555555",
  textLight: "888888",
};

// === FONTS ===
const F = {
  title: "SimHei",
  body: "Microsoft YaHei",
};

// === HELPERS ===
var makeShadow = function () {
  return { type: "outer", blur: 4, offset: 2, color: "000000", opacity: 0.12 };
};

// Chinese quote helpers
var LQ = "“"; // left curly double quote
var RQ = "”"; // right curly double quote

// Content slide with left red accent bar
function addContentSlide(title, bullets, slideNum) {
  var slide = pres.addSlide();
  slide.background = { color: C.white };

  slide.addShape(pres.shapes.RECTANGLE, {
    x: 0, y: 0, w: 0.08, h: 5.625,
    fill: { color: C.red },
  });

  slide.addShape(pres.shapes.RECTANGLE, {
    x: 0.08, y: 0, w: 9.92, h: 0.03,
    fill: { color: C.red },
  });

  slide.addText(title, {
    x: 0.6, y: 0.35, w: 8.8, h: 0.65,
    fontSize: 26, fontFace: F.title, color: C.darkRed, bold: true, margin: 0,
  });

  slide.addShape(pres.shapes.RECTANGLE, {
    x: 0.6, y: 1.0, w: 1.5, h: 0.04,
    fill: { color: C.gold },
  });

  if (bullets && bullets.length > 0) {
    var items = [];
    for (var i = 0; i < bullets.length; i++) {
      items.push({
        text: bullets[i],
        options: {
          bullet: true, breakLine: true,
          fontSize: 15, fontFace: F.body, color: C.textDark, paraSpaceAfter: 10,
        },
      });
    }
    slide.addText(items, {
      x: 0.6, y: 1.3, w: 8.8, h: 3.8,
      valign: "top", margin: 0,
    });
  }

  if (slideNum) {
    slide.addText(String(slideNum), {
      x: 8.8, y: 5.15, w: 0.8, h: 0.35,
      fontSize: 12, fontFace: F.body, color: C.textLight, align: "right",
    });
  }

  return slide;
}

// Section divider slide
function addSectionDivider(number, title) {
  var slide = pres.addSlide();
  slide.background = { color: C.darkRed };

  slide.addText(number, {
    x: 0.6, y: 0.8, w: 3, h: 2.2,
    fontSize: 96, fontFace: F.title, color: C.gold, bold: true,
    margin: 0, transparency: 30,
  });

  slide.addShape(pres.shapes.RECTANGLE, {
    x: 0.6, y: 2.8, w: 2.0, h: 0.05,
    fill: { color: C.gold },
  });

  slide.addText(title, {
    x: 0.6, y: 3.1, w: 8.8, h: 1.2,
    fontSize: 34, fontFace: F.title, color: C.white, bold: true, margin: 0,
  });

  slide.addShape(pres.shapes.RECTANGLE, {
    x: 0, y: 5.5, w: 10, h: 0.125,
    fill: { color: C.gold },
  });

  return slide;
}

// === BUILD SLIDES ===

// ---- SLIDE 1: TITLE ----
(function () {
  var slide = pres.addSlide();
  slide.background = { color: C.darkRed };

  slide.addShape(pres.shapes.RECTANGLE, {
    x: 0, y: 0, w: 10, h: 0.08,
    fill: { color: C.gold },
  });

  slide.addText("入党思想汇报", {
    x: 1.0, y: 1.0, w: 8.0, h: 1.2,
    fontSize: 44, fontFace: F.title, color: C.white, bold: true, margin: 0,
  });

  slide.addShape(pres.shapes.RECTANGLE, {
    x: 1.0, y: 2.2, w: 2.5, h: 0.05,
    fill: { color: C.gold },
  });

  slide.addText("担青年时代使命，正入党初心航向", {
    x: 1.0, y: 2.5, w: 8.0, h: 0.7,
    fontSize: 24, fontFace: F.title, color: C.gold, margin: 0,
  });

  slide.addText(
    "立足" + LQ + "十五五" + RQ + "谋划之年 · 贯彻2026全国两会精神",
    {
      x: 1.0, y: 3.3, w: 8.0, h: 0.5,
      fontSize: 14, fontFace: F.body, color: "E8C9C9", margin: 0,
    }
  );

  slide.addShape(pres.shapes.RECTANGLE, {
    x: 0, y: 5.3, w: 10, h: 0.325,
    fill: { color: C.red },
  });

  slide.addText("思想汇报  |  2026年5月", {
    x: 0.6, y: 5.3, w: 8.8, h: 0.325,
    fontSize: 11, fontFace: F.body, color: "E8C9C9", margin: 0,
  });
})();

// ---- SLIDE 2: TABLE OF CONTENTS ----
(function () {
  var slide = pres.addSlide();
  slide.background = { color: C.white };

  slide.addShape(pres.shapes.RECTANGLE, {
    x: 0, y: 0, w: 0.08, h: 5.625,
    fill: { color: C.red },
  });

  slide.addText("目  录", {
    x: 0.6, y: 0.35, w: 4, h: 0.7,
    fontSize: 30, fontFace: F.title, color: C.darkRed, bold: true, margin: 0,
  });

  slide.addShape(pres.shapes.RECTANGLE, {
    x: 0.6, y: 1.05, w: 1.5, h: 0.04,
    fill: { color: C.gold },
  });

  var tocItems = [
    { num: "一", title: "认清时代形势，把握" + LQ + "十五五" + RQ + "开局的历史方位" },
    { num: "二", title: "勇担时代使命，在新时代书写青春答卷" },
    { num: "三", title: "入党初心的成长与转变" },
    { num: "四", title: "以实际行动向党员标准看齐" },
  ];

  for (var i = 0; i < tocItems.length; i++) {
    var item = tocItems[i];
    var yBase = 1.45 + i * 0.95;

    slide.addShape(pres.shapes.OVAL, {
      x: 0.6, y: yBase + 0.05, w: 0.52, h: 0.52,
      fill: { color: C.red },
    });

    slide.addText(item.num, {
      x: 0.6, y: yBase + 0.05, w: 0.52, h: 0.52,
      fontSize: 20, fontFace: F.title, color: C.white, bold: true,
      align: "center", valign: "middle", margin: 0,
    });

    slide.addText(item.title, {
      x: 1.4, y: yBase, w: 7.5, h: 0.6,
      fontSize: 18, fontFace: F.body, color: C.textDark,
      valign: "middle", margin: 0,
    });

    if (i < tocItems.length - 1) {
      slide.addShape(pres.shapes.LINE, {
        x: 1.4, y: yBase + 0.75, w: 7.5, h: 0,
        line: { color: "E8E0E0", width: 0.75 },
      });
    }
  }
})();

// ---- SLIDE 3: PREFACE ----
addContentSlide(
  "前言：时代召唤，使命在肩",
  [
    "当前我国正处于" + LQ + "十五五" + RQ + "规划谋划布局的关键时期，面对百年未有之大变局，中华民族伟大复兴进入不可逆转的历史进程。",
    "作为新时代青年，既要深刻认识时代赋予的机遇与挑战，也要时刻校正自己的定位与方向。",
    "结合2026全国两会精神，结合个人专业学习、实践锻炼与职业规划，回顾入党初心从朴素情感到深刻认同、从感性到理性的转变过程。",
    "旨在进一步强化对党宗旨的深刻理解与认同，为加入党组织做好思想准备和实践准备。",
  ],
  3
);

// ---- SLIDE 4: Section 1 Divider ----
addSectionDivider(
  "一",
  "认清时代形势，把握" + LQ + "十五五" + RQ + "开局的历史方位"
);

// ---- SLIDE 5: 1-1 ----
addContentSlide(
  "（一）国际国内形势的深刻变化",
  [
    "当前国际格局正发生深刻复杂变化，大国博弈加剧，地区冲突不断，全球化发展遭遇逆流。中国始终保持战略定力，坚持和平发展道路，积极推动构建人类命运共同体。",
    "国内经济社会发展迈上新台阶，高质量发展成为主题，新质生产力加快形成，科技突破和产业升级为青年一代提供了广阔的发展舞台。",
    "也要清醒看到发展不平衡不充分问题仍然突出，关键核心技术受制于人，青年一代应担当起突破责任，积极投身科技创新和产业升级的实践中去。",
  ],
  5
);

// ---- SLIDE 6: 1-2 ----
addContentSlide(
  "（二）全国两会精神的核心要义",
  [
    "2026年全国两会在" + LQ + "十五五" + RQ + "规划谋划之年召开，聚焦推动高质量发展，强调科技自立自强，推进教育科技人才一体化发展。",
    "会议部署加快发展新质生产力，壮大新兴产业，推动产业创新应用——这些部署与青年一代的成长发展息息相关。",
    "作为新时代青年，要深入学习两会精神，把个人成长方向与国家发展战略紧密结合，在服务国家发展中实现人生价值。",
  ],
  6
);

// ---- SLIDE 7: 1-3 ----
addContentSlide(
  "（三）" + LQ + "十五五" + RQ + "规划的重大意义",
  [
    LQ + "十五五" + RQ + "时期是我国全面建设社会主义现代化国家的关键时期，规划明确了未来五年经济社会发展的主要目标和重点任务。",
    "规划强调生态文明建设、绿色转型、共同富裕等核心议题，为国家发展指明了方向，也为每一个青年人的成长提供了宏阔背景。",
    "青年人要深刻理解" + LQ + "十五五" + RQ + "规划的重大意义，自觉将个人发展融入国家发展大局之中。",
  ],
  7
);

// ---- SLIDE 8: Section 2 Divider ----
addSectionDivider("二", "勇担时代使命，在新时代书写青春答卷");

// ---- SLIDE 9: 2-1 ----
addContentSlide(
  "（一）新时代青年的历史责任",
  [
    "党的二十大报告指出：" + LQ + "当代中国青年生逢其时，施展才干的舞台无比广阔，实现梦想的前景无比光明。" + RQ,
    "新时代青年既要有家国情怀，也要有国际视野；既要仰望星空，也要脚踏实地。",
    "作为新时代青年，更应走在时代前列、争当先锋模范，为民族复兴贡献青春力量，这是时代赋予我们的历史使命。",
  ],
  9
);

// ---- SLIDE 10: 2-2 ----
addContentSlide(
  "（二）在专业学习中练就过硬本领",
  [
    "作为新时代的大学生，专业学习是我们的立身之本。要刻苦钻研专业知识，培养创新思维和解决实际问题的能力。",
    "要将所学专业与历史使命联系起来，关注国家重大战略需求，使自己的学识真正服务于国家发展需要。",
    "要积极参加社会实践，在实践中检验学识、增长才干，做到知行合一。",
  ],
  10
);

// ---- SLIDE 11: 2-3 ----
addContentSlide(
  "（三）以职业规划与国家发展同频共振",
  [
    "职业规划不是纯粹的个人设计，而应与国家发展同向同行。要关注国家产业发展趋势，了解行业前沿动态。",
    "在职业选择中体现家国情怀和责任担当，无论是投身科技创新还是服务基层，都是为国家贡献力量的重要岗位。",
    "要把个人理想融入党和国家的事业之中，在服务国家发展中找准职业定位，实现个人价值与社会价值的统一。",
  ],
  11
);

// ---- SLIDE 12: Section 3 Divider ----
addSectionDivider("三", "入党初心的成长与转变");

// ---- SLIDE 13: 3-1 ----
addContentSlide(
  "（一）从朴素情感到深刻认同",
  [
    "回顾入党动机，很多同学都是基于对党的朴素情感——受家庭环境襀陶、被优秀党员事迹感染、在学习党史中产生敬仰。这些朴素的情感是可贵的。",
    "随着理论学习深入和思想成熟，我逐渐认识到：加入党组织不是一种荣誉的加冕，而是一份沉甸甸的责任。",
    "党的根本宗旨是全心全意为人民服务，党员的价值在于奉献而非索取。实现从朴素情感到深刻认同的转变，是思想成熟的重要标志。",
  ],
  13
);

// ---- SLIDE 14: 3-2 + 3-3 combined ----
addContentSlide(
  "（二）从感性认识到坚定信念  /  （三）对党宗旨的深刻理解",
  [
    "坚定理想信念来源于深刻的理论理解。通过系统学习党的理论、党史和党章党规，参加党校培训、志愿服务、社会实践等活动，我直观感受到党的理论与实践的统一。",
    "这种从感性到理性的飞跃，使入党动机更加纯净、更加坚定。我深刻认识到：只有中国共产党才能领导中国实现民族复兴。",
    LQ + "全心全意为人民服务" + RQ + "不是一句口号，而是落实在点点滴滴的行动中。必须将其内化为自己的价值追求和行动自觉。",
  ],
  14
);

// ---- SLIDE 15: Section 4 Divider ----
addSectionDivider("四", "以实际行动向党员标准看齐");

// ---- SLIDE 16: Three cards for section 4 ----
(function () {
  var slide = pres.addSlide();
  slide.background = { color: C.white };

  slide.addShape(pres.shapes.RECTANGLE, {
    x: 0, y: 0, w: 0.08, h: 5.625,
    fill: { color: C.red },
  });

  slide.addShape(pres.shapes.RECTANGLE, {
    x: 0.08, y: 0, w: 9.92, h: 0.03,
    fill: { color: C.red },
  });

  slide.addText("以实际行动向党员标准看齐", {
    x: 0.6, y: 0.35, w: 8.8, h: 0.65,
    fontSize: 26, fontFace: F.title, color: C.darkRed, bold: true, margin: 0,
  });

  slide.addShape(pres.shapes.RECTANGLE, {
    x: 0.6, y: 1.0, w: 1.5, h: 0.04,
    fill: { color: C.gold },
  });

  var cards = [
    {
      title: "思想成长",
      sub: "筑牢信仰之基",
      text: "坚持学习习近平新时代中国特色社会主义思想，增强" + LQ + "四个意识" + RQ + "、坚定" + LQ + "四个自信" + RQ + "、做到" + LQ + "两个维护" + RQ + "，经常开展批评与自我批评。",
      color: C.red,
    },
    {
      title: "实践锻炼",
      sub: "在服务中增长才干",
      text: "积极参加志愿服务、社会实践、科技创新等活动，在服务同学、服务班级中锻炼组织协调能力，弘扬理论联系实际的学风。",
      color: C.gold,
    },
    {
      title: "行动对标",
      sub: "以党员先进性鞭策自己",
      text: "学习上刻苦努力，工作上勇于担当，生活上严于律己。把先进典型作为镜子对照自己，用党的纪律规矩严格约束自己。",
      color: C.darkRed,
    },
  ];

  for (var i = 0; i < cards.length; i++) {
    var card = cards[i];
    var xBase = 0.5 + i * 3.1;
    var yBase = 1.4;

    slide.addShape(pres.shapes.RECTANGLE, {
      x: xBase, y: yBase, w: 2.85, h: 3.6,
      fill: { color: C.lightBg },
      shadow: makeShadow(),
    });

    slide.addShape(pres.shapes.RECTANGLE, {
      x: xBase, y: yBase, w: 2.85, h: 0.06,
      fill: { color: card.color },
    });

    slide.addText(card.title, {
      x: xBase + 0.2, y: yBase + 0.25, w: 2.45, h: 0.45,
      fontSize: 20, fontFace: F.title, color: C.darkRed, bold: true, margin: 0,
    });

    slide.addText(card.sub, {
      x: xBase + 0.2, y: yBase + 0.7, w: 2.45, h: 0.35,
      fontSize: 13, fontFace: F.body, color: C.textMedium, margin: 0,
    });

    slide.addShape(pres.shapes.RECTANGLE, {
      x: xBase + 0.2, y: yBase + 1.1, w: 1.0, h: 0.025,
      fill: { color: "E0D0D0" },
    });

    slide.addText(card.text, {
      x: xBase + 0.2, y: yBase + 1.3, w: 2.45, h: 2.0,
      fontSize: 12.5, fontFace: F.body, color: C.textDark,
      lineSpacingMultiple: 1.5, margin: 0,
    });
  }

  slide.addText("16", {
    x: 8.8, y: 5.15, w: 0.8, h: 0.35,
    fontSize: 12, fontFace: F.body, color: C.textLight, align: "right",
  });
})();

// ---- SLIDE 17: CONCLUSION ----
(function () {
  var slide = pres.addSlide();
  slide.background = { color: C.darkRed };

  slide.addShape(pres.shapes.RECTANGLE, {
    x: 0, y: 0, w: 10, h: 0.08,
    fill: { color: C.gold },
  });

  slide.addText("结语：坚定信心、牢记使命", {
    x: 0.8, y: 0.4, w: 8.4, h: 0.8,
    fontSize: 32, fontFace: F.title, color: C.white, bold: true, margin: 0,
  });

  slide.addShape(pres.shapes.RECTANGLE, {
    x: 0.8, y: 1.2, w: 2.0, h: 0.05,
    fill: { color: C.gold },
  });

  slide.addText(
    "青春向党，时代向上。加入中国共产党不是一时的冲动，而是一生的承诺。",
    {
      x: 0.8, y: 1.5, w: 8.4, h: 0.6,
      fontSize: 16, fontFace: F.body, color: "F0D0D0", italic: true, margin: 0,
    }
  );

  var outlooks = [
    "持续加强理论学习，用习近平新时代中国特色社会主义思想武装头脑，不断提升政治理论水平。",
    "勤奋刻苦，练就过硬本领，在专业学习和社会实践中增长才干，在服务中绽放青春。",
    "时刻对标党员标准，严格自我要求，积极向组织靠拢，争取早日成为一名合格的共产党员。",
    "牢记初心使命，矢志不渝，始终保持对党忠诚和对人民的热爱，在为人民服务的道路上坚定前行。",
  ];

  var outlookItems = [];
  for (var j = 0; j < outlooks.length; j++) {
    outlookItems.push({
      text: outlooks[j],
      options: {
        bullet: { type: "number" }, breakLine: true,
        fontSize: 13, fontFace: F.body, color: C.white, paraSpaceAfter: 8,
      },
    });
  }

  slide.addText(outlookItems, {
    x: 0.8, y: 2.3, w: 8.4, h: 2.6,
    valign: "top", margin: 0,
  });

  slide.addShape(pres.shapes.RECTANGLE, {
    x: 0, y: 5.3, w: 10, h: 0.325,
    fill: { color: C.red },
  });

  slide.addText("谢谢！", {
    x: 0, y: 5.3, w: 10, h: 0.325,
    fontSize: 12, fontFace: F.body, color: "E8C9C9", align: "center", margin: 0,
  });
})();

// === WRITE FILE ===
pres
  .writeFile({ fileName: "d:\\huosheng\\入党思想汇报_担青年时代使命正入党初心航向.pptx" })
  .then(function () { console.log("PPTX created successfully!"); })
  .catch(function (err) { console.error("Error:", err); });
